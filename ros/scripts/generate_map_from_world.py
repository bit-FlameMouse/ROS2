#!/usr/bin/env python3
# 文件用途：由 turtlebot3_gazebo 的世界几何生成栅格地图（.pgm + .yaml）
#
# 背景：本项目的地图应通过 mapping.launch.py + SLAM Toolbox 采集（见 09 文档 §6）。
#       本脚本是 10 文档 §6 降级预案 P2 的工程化实现：在无法运行 Gazebo 的
#       环境中，直接解析世界模型（墙体 mesh + 圆柱 + 装饰 mesh）的碰撞几何，
#       离线光栅化为占据栅格，保证“地图与 Gazebo 世界障碍布局一致”（AC-03）。
#       正式演示前建议用 mapping.launch.py 重新采集并覆盖本文件产物。
#
# 用法：
#   python3 scripts/generate_map_from_world.py --output src/patrol_robot_bringup/maps/tb3_world
#   （可选 --model-sdf 显式指定 turtlebot3_world/model.sdf 路径）
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
import argparse
import math
import os
import sys
import xml.etree.ElementTree as ET

CG = 254  # 空闲
CO = 0    # 占据
CU = 205  # 未知


def load_dae_polygons(dae_path, scale):
    """解析 DAE，返回 (外多边形, 内多边形或 None)（单位：米，已应用缩放）。"""
    tree = ET.parse(dae_path)
    ns = {'c': 'http://www.collada.org/2005/11/COLLADASchema'}
    unit = 1.0
    asset = tree.getroot().find('c:asset', ns)
    if asset is not None:
        u = asset.find('c:unit', ns)
        if u is not None and u.get('meter'):
            unit = float(u.get('meter'))

    pts = set()
    for vertices in tree.getroot().iter('{http://www.collada.org/2005/11/COLLADASchema}vertices'):
        position_id = None
        for inp in vertices.findall('{http://www.collada.org/2005/11/COLLADASchema}input'):
            if inp.get('semantic') == 'POSITION':
                position_id = inp.get('source', '').lstrip('#')
        if not position_id:
            continue
        src = tree.getroot().find(
            f'.//{{http://www.collada.org/2005/11/COLLADASchema}}source[@id="{position_id}"]')
        fa = src.find('{http://www.collada.org/2005/11/COLLADASchema}float_array') if src is not None else None
        if fa is None or not fa.text:
            continue
        vals = [float(v) for v in fa.text.split()]
        if len(vals) % 3 != 0:
            continue
        for i in range(0, len(vals), 3):
            pts.add((round(vals[i] * unit * scale, 6), round(vals[i + 1] * unit * scale, 6)))

    unique = sorted(pts)
    if len(unique) < 3:
        return unique, None
    hull = convex_hull(unique)
    inner_pts = [p for p in unique if point_in_polygon(p[0], p[1], hull) == 0]
    inner = convex_hull(inner_pts) if len(inner_pts) >= 3 else None
    return hull, inner


def convex_hull(points):
    pts = sorted(set(points))

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    lower = []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 1e-12:
            lower.pop()
        lower.append(p)
    upper = []
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 1e-12:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def point_in_polygon(px, py, poly):
    """凸多边形位置判定：0=内部，1=边界，-1=外部。"""
    has_pos = False
    has_neg = False
    has_zero = False
    n = len(poly)
    for i in range(n):
        x1, y1 = poly[i]
        x2, y2 = poly[(i + 1) % n]
        v = (x2 - x1) * (py - y1) - (y2 - y1) * (px - x1)
        if v > 1e-12:
            has_pos = True
        elif v < -1e-12:
            has_neg = True
        else:
            has_zero = True
    if has_pos and has_neg:
        return -1
    if has_zero:
        return 1
    return 0


def polygon_contains(px, py, poly):
    return point_in_polygon(px, py, poly) != -1


class Shape:
    """一个碰撞体（圆 / 多边形 / 多边形环）。"""

    def __init__(self, kind, **kwargs):
        self.kind = kind
        self.__dict__.update(kwargs)

    def contains(self, x, y):
        if self.kind == 'circle':
            dx, dy = x - self.cx, y - self.cy
            return dx * dx + dy * dy <= self.r * self.r
        if self.kind == 'polygon':
            return polygon_contains(x, y, self.outer)
        if self.kind == 'ring':
            return (polygon_contains(x, y, self.outer) and
                    not polygon_contains(x, y, self.inner))
        return False

    def bounds(self):
        if self.kind == 'circle':
            return (self.cx - self.r, self.cy - self.r, self.cx + self.r, self.cy + self.r)
        xs = [p[0] for p in self.outer]
        ys = [p[1] for p in self.outer]
        return (min(xs), min(ys), max(xs), max(ys))


def parse_pose(text):
    vals = [float(v) for v in (text or '0 0 0 0 0 0').split()]
    while len(vals) < 6:
        vals.append(0.0)
    return vals


def mesh_path(model_dir, uri):
    prefix = 'model://'
    rel = uri[len(prefix):] if uri.startswith(prefix) else uri
    # rel 形如 <model_name>/meshes/xxx.dae，去掉模型名取相对路径
    parts = rel.split('/', 1)
    return os.path.join(model_dir, parts[1] if len(parts) > 1 else rel)


def load_shapes(model_dir, sdf_path, default_scale):
    root = ET.parse(sdf_path).getroot()
    shapes = []
    for collision in root.iter('collision'):
        px, py = parse_pose(collision.findtext('pose'))[:2]
        geom = collision.find('geometry')
        if geom is None:
            continue
        cyl = geom.find('cylinder')
        box = geom.find('box')
        mesh = geom.find('mesh')
        if cyl is not None:
            r = float(cyl.findtext('radius', '0')) * default_scale
            shapes.append(Shape('circle', cx=px, cy=py, r=r))
        elif box is not None:
            sx, sy = [float(v) * default_scale for v in box.findtext('size').split()[:2]]
            shapes.append(Shape(
                'polygon',
                outer=[(px + dx, py + dy) for dx, dy in
                       [(-sx / 2, -sy / 2), (sx / 2, -sy / 2), (sx / 2, sy / 2), (-sx / 2, sy / 2)]]))
        elif mesh is not None:
            scale_text = mesh.findtext('scale', '1 1 1').split()
            scale = float(scale_text[0]) * default_scale
            outer, inner = load_dae_polygons(mesh_path(model_dir, mesh.findtext('uri')), scale)
            # 只支持绕 z 的旋转（本项目世界满足）
            yaw = parse_pose(collision.findtext('pose'))[5]
            cos_y, sin_y = math.cos(yaw), math.sin(yaw)

            def transform(p):
                return (px + p[0] * cos_y - p[1] * sin_y,
                        py + p[0] * sin_y + p[1] * cos_y)

            outer_t = [transform(p) for p in outer]
            if inner is None:
                shapes.append(Shape('polygon', outer=outer_t))
            else:
                shapes.append(Shape('ring', outer=outer_t,
                                    inner=[transform(p) for p in inner]))
    return shapes


def rasterize(shapes, resolution, margin, seed=(0.5, 0.5)):
    x_min = min(s.bounds()[0] for s in shapes) - margin
    y_min = min(s.bounds()[1] for s in shapes) - margin
    x_max = max(s.bounds()[2] for s in shapes) + margin
    y_max = max(s.bounds()[3] for s in shapes) + margin

    x_min = math.floor(x_min / resolution) * resolution
    y_min = math.floor(y_min / resolution) * resolution
    width = int(math.ceil((x_max - x_min) / resolution))
    height = int(math.ceil((y_max - y_min) / resolution))

    grid = []
    for row in range(height):
        wy = y_max - (row + 0.5) * resolution
        line = bytearray(width)
        for col in range(width):
            wx = x_min + (col + 0.5) * resolution
            line[col] = CO if any(s.contains(wx, wy) for s in shapes) else CU
        grid.append(line)

    # 房间内部（被墙环/障碍包围的可达区域）标记为空闲：
    # 从房间内一个已知空点做 4 连通洪泛填充，填充不出去的区域保持“未知”
    cx = int((seed[0] - x_min) / resolution)
    cy = int((y_max - seed[1]) / resolution)
    if 0 <= cx < width and 0 <= cy < height and grid[cy][cx] == CU:
        stack = [(cx, cy)]
        while stack:
            x, y = stack.pop()
            if x < 0 or y < 0 or x >= width or y >= height or grid[y][x] != CU:
                continue
            grid[y][x] = CG
            stack.extend([(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)])
    return grid, x_min, y_min, width, height


def write_outputs(grid, x_min, y_min, width, height, resolution, output_prefix):
    pgm_path = output_prefix + '.pgm'
    yaml_path = output_prefix + '.yaml'
    with open(pgm_path, 'wb') as f:
        f.write(b'P5\n# CREATOR: generate_map_from_world.py (tb3 world geometry)\n')
        f.write(f'{width} {height}\n255\n'.encode())
        for row in grid:
            f.write(bytes(row))
    with open(yaml_path, 'w') as f:
        f.write('# 由世界几何离线生成（见 scripts/generate_map_from_world.py 文件头说明）\n')
        f.write('image: ' + os.path.basename(pgm_path) + '\n')
        f.write('mode: trinary\n')
        f.write(f'resolution: {resolution:.6f}\n')
        f.write(f'origin: [{x_min:.6f}, {y_min:.6f}, 0.000000]\n')
        f.write('negate: 0\n')
        f.write('occupied_thresh: 0.65\n')
        f.write('free_thresh: 0.25\n')
    return pgm_path, yaml_path


def main():
    parser = argparse.ArgumentParser(description='由 TB3 世界几何生成栅格地图')
    parser.add_argument('--model-sdf', help='turtlebot3_world/model.sdf 路径')
    parser.add_argument('--output', default='src/patrol_robot_bringup/maps/tb3_world',
                        help='输出前缀（不含扩展名）')
    parser.add_argument('--resolution', type=float, default=0.05, help='分辨率 m/px')
    parser.add_argument('--margin', type=float, default=0.6, help='地图边界外扩（m）')
    args = parser.parse_args()

    model_sdf = args.model_sdf
    if not model_sdf:
        try:
            from ament_index_python.packages import get_package_share_directory
            model_sdf = os.path.join(
                get_package_share_directory('turtlebot3_gazebo'),
                'models', 'turtlebot3_world', 'model.sdf')
        except Exception as exc:  # noqa: BLE001 - 需要给出可操作的错误提示
            print('错误：未找到 turtlebot3_gazebo，请指定 --model-sdf。', file=sys.stderr)
            print(f'原因：{exc}', file=sys.stderr)
            return 1

    model_dir = os.path.dirname(model_sdf)
    if not os.path.isfile(model_sdf):
        print(f'错误：模型文件不存在：{model_sdf}', file=sys.stderr)
        return 1

    # 世界文件中 <model name="turtlebot3_world"> 的 include 缩放为 1，
    # 单体模型内部的 mesh/cylinder 已在模型坐标系下（默认缩放 1）
    shapes = load_shapes(model_dir, model_sdf, default_scale=1.0)
    grid, x_min, y_min, width, height = rasterize(shapes, args.resolution, args.margin)
    pgm, yml = write_outputs(
        grid, x_min, y_min, width, height, args.resolution, args.output)

    occupied = sum(row.count(CO) for row in grid)
    print(f'几何体数量: {len(shapes)}')
    print(f'地图尺寸: {width} x {height} px, 分辨率 {args.resolution} m/px')
    print(f'原点: ({x_min:.2f}, {y_min:.2f}), 范围: '
          f'x[{x_min:.2f}, {x_min + width * args.resolution:.2f}] '
          f'y[{y_min:.2f}, {y_min + height * args.resolution:.2f}]')
    print(f'占据栅格: {occupied} ({occupied * 100.0 / (width * height):.1f}%)')
    print(f'已写出: {pgm}')
    print(f'已写出: {yml}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
