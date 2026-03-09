use egui::{
    Align2, Color32, Context, CornerRadius, FontId, Id, LayerId, Order, Pos2, RawInput, Rect,
    Stroke, StrokeKind, Vec2,
};
use std::env;
use std::fs::OpenOptions;
use std::io::Write;
use std::path::Path;
use std::time::Instant;

const LABEL_CAP: usize = 32;

#[derive(Copy, Clone, Eq, PartialEq)]
enum Scenario {
    Static,
    Sparse,
    FullHot,
}

#[derive(Copy, Clone, Eq, PartialEq)]
enum SceneKind {
    Cards,
    TextWall,
    Graph,
    Inspector,
    Hierarchy,
}

struct Config {
    width: i32,
    height: i32,
    count: usize,
    mutate_count: usize,
    columns: usize,
    seconds: f64,
    warmup_seconds: f64,
    cell_width: f32,
    cell_height: f32,
    gap: f32,
    csv_path: Option<String>,
    mode_name: &'static str,
    scene_kind: SceneKind,
    scene_name: &'static str,
    scenario: Scenario,
    scenario_name: &'static str,
}

struct FrameMetrics {
    frame_ms: f64,
    command_count: u64,
    vertex_count: u64,
    index_count: u64,
    text_bytes: u64,
}

struct Accumulator {
    frames: u64,
    total_frame_ms: f64,
    min_frame_ms: f64,
    max_frame_ms: f64,
    total_wall_seconds: f64,
    total_command_count: u64,
    total_vertex_count: u64,
    total_index_count: u64,
    total_text_bytes: u64,
}

struct Scene {
    labels: Vec<String>,
    total_text_bytes: u64,
}

fn default_config() -> Config {
    Config {
        width: 1920,
        height: 1080,
        count: 10_000,
        mutate_count: 100,
        columns: 16,
        seconds: 2.0,
        warmup_seconds: 1.0,
        cell_width: 112.0,
        cell_height: 34.0,
        gap: 6.0,
        csv_path: None,
        mode_name: "headless-primitive",
        scene_kind: SceneKind::Cards,
        scene_name: "cards",
        scenario: Scenario::Static,
        scenario_name: "static",
    }
}

fn usage(program_name: &str) {
    eprintln!(
        "usage: {program_name} [--scene cards|textwall|graph|inspector|hierarchy] [--scenario static|sparse|fullhot] [--count N] [--mutate-count N]"
    );
    eprintln!("          [--seconds S] [--warmup-seconds S] [--columns N] [--width W] [--height H]");
    eprintln!("          [--csv path]");
}

fn parse_scene_kind(value: &str) -> SceneKind {
    match value {
        "cards" => SceneKind::Cards,
        "textwall" => SceneKind::TextWall,
        "graph" => SceneKind::Graph,
        "inspector" => SceneKind::Inspector,
        "hierarchy" => SceneKind::Hierarchy,
        _ => {
            eprintln!("unknown scene: {value}");
            std::process::exit(2);
        }
    }
}

fn parse_scenario(value: &str) -> Scenario {
    match value {
        "static" => Scenario::Static,
        "sparse" => Scenario::Sparse,
        "fullhot" => Scenario::FullHot,
        _ => {
            eprintln!("unknown scenario: {value}");
            std::process::exit(2);
        }
    }
}

fn scene_kind_name(scene_kind: SceneKind) -> &'static str {
    match scene_kind {
        SceneKind::Cards => "cards",
        SceneKind::TextWall => "textwall",
        SceneKind::Graph => "graph",
        SceneKind::Inspector => "inspector",
        SceneKind::Hierarchy => "hierarchy",
    }
}

fn scenario_name(scenario: Scenario) -> &'static str {
    match scenario {
        Scenario::Static => "static",
        Scenario::Sparse => "sparse",
        Scenario::FullHot => "fullhot",
    }
}

fn parse_args() -> Config {
    let mut cfg = default_config();
    let args: Vec<String> = env::args().collect();
    let mut i = 1usize;
    while i < args.len() {
        match args[i].as_str() {
            "--scene" if i + 1 < args.len() => {
                cfg.scene_kind = parse_scene_kind(&args[i + 1]);
                i += 2;
            }
            "--scenario" if i + 1 < args.len() => {
                cfg.scenario = parse_scenario(&args[i + 1]);
                i += 2;
            }
            "--count" if i + 1 < args.len() => {
                cfg.count = args[i + 1].parse().unwrap_or(0);
                i += 2;
            }
            "--mutate-count" if i + 1 < args.len() => {
                cfg.mutate_count = args[i + 1].parse().unwrap_or(0);
                i += 2;
            }
            "--seconds" if i + 1 < args.len() => {
                cfg.seconds = args[i + 1].parse().unwrap_or(0.0);
                i += 2;
            }
            "--warmup-seconds" if i + 1 < args.len() => {
                cfg.warmup_seconds = args[i + 1].parse().unwrap_or(0.0);
                i += 2;
            }
            "--columns" if i + 1 < args.len() => {
                cfg.columns = args[i + 1].parse().unwrap_or(0);
                i += 2;
            }
            "--width" if i + 1 < args.len() => {
                cfg.width = args[i + 1].parse().unwrap_or(0);
                i += 2;
            }
            "--height" if i + 1 < args.len() => {
                cfg.height = args[i + 1].parse().unwrap_or(0);
                i += 2;
            }
            "--csv" if i + 1 < args.len() => {
                cfg.csv_path = Some(args[i + 1].clone());
                i += 2;
            }
            "--mode" if i + 1 < args.len() => {
                cfg.mode_name = Box::leak(args[i + 1].clone().into_boxed_str());
                i += 2;
            }
            _ => {
                usage(&args[0]);
                std::process::exit(2);
            }
        }
    }

    if cfg.count == 0 || cfg.columns == 0 || cfg.seconds <= 0.0 || cfg.warmup_seconds < 0.0 {
        eprintln!("bad benchmark config");
        std::process::exit(2);
    }
    cfg.mutate_count = cfg.mutate_count.min(cfg.count);
    cfg.scene_name = scene_kind_name(cfg.scene_kind);
    cfg.scenario_name = scenario_name(cfg.scenario);
    cfg
}

fn accum_reset() -> Accumulator {
    Accumulator {
        frames: 0,
        total_frame_ms: 0.0,
        min_frame_ms: f64::MAX,
        max_frame_ms: 0.0,
        total_wall_seconds: 0.0,
        total_command_count: 0,
        total_vertex_count: 0,
        total_index_count: 0,
        total_text_bytes: 0,
    }
}

fn accum_add(acc: &mut Accumulator, frame: &FrameMetrics) {
    acc.frames += 1;
    acc.total_frame_ms += frame.frame_ms;
    acc.min_frame_ms = acc.min_frame_ms.min(frame.frame_ms);
    acc.max_frame_ms = acc.max_frame_ms.max(frame.frame_ms);
    acc.total_command_count += frame.command_count;
    acc.total_vertex_count += frame.vertex_count;
    acc.total_index_count += frame.index_count;
    acc.total_text_bytes += frame.text_bytes;
}

fn average_or_zero(total: f64, count: u64) -> f64 {
    if count == 0 {
        0.0
    } else {
        total / count as f64
    }
}

fn average_u64_or_zero(total: u64, count: u64) -> u64 {
    if count == 0 {
        0
    } else {
        total / count
    }
}

fn item_is_hot(cfg: &Config, frame_index: u64, item_index: usize) -> bool {
    if cfg.scenario == Scenario::FullHot {
        return true;
    }
    if cfg.scenario != Scenario::Sparse || cfg.mutate_count == 0 {
        return false;
    }
    let start = ((frame_index * cfg.mutate_count as u64) % cfg.count as u64) as usize;
    let end = start + cfg.mutate_count;
    if end <= cfg.count {
        item_index >= start && item_index < end
    } else {
        item_index >= start || item_index < (end - cfg.count)
    }
}

fn rgba_u32(r: u8, g: u8, b: u8, a: u8) -> u32 {
    ((r as u32) << 24) | ((g as u32) << 16) | ((b as u32) << 8) | a as u32
}

fn color32_from_u32(rgba: u32) -> Color32 {
    Color32::from_rgba_unmultiplied(
        ((rgba >> 24) & 0xff) as u8,
        ((rgba >> 16) & 0xff) as u8,
        ((rgba >> 8) & 0xff) as u8,
        (rgba & 0xff) as u8,
    )
}

fn fill_color(cfg: &Config, frame_index: u64, item_index: usize) -> u32 {
    let hot = item_is_hot(cfg, frame_index, item_index);
    let mut r = 26 + ((item_index % 5) as u8) * 8;
    let mut g = 31 + ((item_index % 7) as u8) * 6;
    let mut b = 39 + ((item_index % 11) as u8) * 5;
    if hot {
        r = 60 + ((frame_index + item_index as u64 * 3) % 120) as u8;
        g = 110 + ((frame_index + item_index as u64 * 5) % 100) as u8;
        b = 180 + ((frame_index + item_index as u64 * 7) % 60) as u8;
    }
    rgba_u32(r, g, b, 255)
}

fn text_color() -> u32 {
    rgba_u32(230, 233, 240, 255)
}

fn hot_text_color() -> u32 {
    rgba_u32(163, 214, 255, 255)
}

fn wire_color() -> u32 {
    rgba_u32(74, 158, 255, 220)
}

fn hot_wire_color() -> u32 {
    rgba_u32(110, 210, 255, 255)
}

fn inspector_value_fill_color(cfg: &Config, frame_index: u64, item_index: usize) -> u32 {
    if item_is_hot(cfg, frame_index, item_index) {
        hot_wire_color()
    } else {
        rgba_u32(49, 55, 70, 255)
    }
}

fn cell_rect(cfg: &Config, item_index: usize) -> Rect {
    let row = item_index / cfg.columns;
    let column = item_index % cfg.columns;
    let x = cfg.gap + column as f32 * (cfg.cell_width + cfg.gap);
    let y = cfg.gap + row as f32 * (cfg.cell_height + cfg.gap);
    Rect::from_min_size(Pos2::new(x, y), Vec2::new(cfg.cell_width, cfg.cell_height))
}

fn text_line_rect(cfg: &Config, item_index: usize) -> Rect {
    let line_height = 18.0;
    let column_width = 248.0;
    let row = item_index / cfg.columns;
    let column = item_index % cfg.columns;
    let x = 14.0 + column as f32 * (column_width + 24.0);
    let y = 14.0 + row as f32 * line_height;
    Rect::from_min_size(Pos2::new(x, y), Vec2::new(column_width, line_height))
}

fn graph_node_rect(cfg: &Config, item_index: usize) -> Rect {
    let node_width = 136.0;
    let node_height = 78.0;
    let pitch_x = 172.0;
    let pitch_y = 112.0;
    let row = item_index / cfg.columns;
    let column = item_index % cfg.columns;
    let stagger = if (row & 1) == 1 { 18.0 } else { -18.0 };
    let x = 28.0 + column as f32 * pitch_x + stagger;
    let y = 24.0 + row as f32 * pitch_y + (column % 3) as f32 * 7.0;
    Rect::from_min_size(Pos2::new(x, y), Vec2::new(node_width, node_height))
}

fn inspector_row_rect(cfg: &Config, item_index: usize) -> Rect {
    let columns = cfg.columns.clamp(1, 4);
    let row_height = 28.0;
    let pane_width = 360.0;
    let pane_gap = 32.0;
    let row = item_index / columns;
    let column = item_index % columns;
    let x = 20.0 + column as f32 * (pane_width + pane_gap);
    let y = 18.0 + row as f32 * row_height;
    Rect::from_min_size(Pos2::new(x, y), Vec2::new(pane_width, 24.0))
}

fn hierarchy_row_rect(cfg: &Config, item_index: usize) -> Rect {
    let columns = cfg.columns.clamp(1, 3);
    let row_height = 24.0;
    let pane_width = 320.0;
    let pane_gap = 26.0;
    let row = item_index / columns;
    let column = item_index % columns;
    let x = 18.0 + column as f32 * (pane_width + pane_gap);
    let y = 18.0 + row as f32 * row_height;
    Rect::from_min_size(Pos2::new(x, y), Vec2::new(pane_width, 20.0))
}

fn scene_rect(cfg: &Config, item_index: usize) -> Rect {
    match cfg.scene_kind {
        SceneKind::Cards => cell_rect(cfg, item_index),
        SceneKind::TextWall => text_line_rect(cfg, item_index),
        SceneKind::Graph => graph_node_rect(cfg, item_index),
        SceneKind::Inspector => inspector_row_rect(cfg, item_index),
        SceneKind::Hierarchy => hierarchy_row_rect(cfg, item_index),
    }
}

fn hierarchy_depth(item_index: usize) -> usize {
    let mut depth = 0usize;
    let mut cursor = item_index;
    while cursor % 7 != 0 && depth < 4 {
        depth += 1;
        cursor /= 2;
    }
    depth
}

fn hierarchy_has_children(item_index: usize) -> bool {
    item_index % 5 != 4
}

fn hierarchy_row_fill_color(cfg: &Config, frame_index: u64, item_index: usize) -> u32 {
    if item_is_hot(cfg, frame_index, item_index) {
        rgba_u32(38, 66, 103, 255)
    } else if item_index % 6 == 0 {
        rgba_u32(28, 33, 42, 255)
    } else {
        rgba_u32(22, 26, 33, 220)
    }
}

fn hierarchy_marker_fill_color(cfg: &Config, frame_index: u64, item_index: usize) -> u32 {
    if item_is_hot(cfg, frame_index, item_index) {
        hot_wire_color()
    } else {
        rgba_u32(74, 86, 104, 255)
    }
}

fn hierarchy_kind_fill_color(item_index: usize) -> u32 {
    match item_index % 4 {
        0 => rgba_u32(49, 55, 70, 255),
        1 => rgba_u32(58, 70, 92, 255),
        2 => rgba_u32(61, 79, 63, 255),
        _ => rgba_u32(82, 64, 45, 255),
    }
}

fn hierarchy_kind_name(item_index: usize) -> &'static str {
    match item_index % 4 {
        0 => "Mesh",
        1 => "Light",
        2 => "Group",
        _ => "Cam",
    }
}

fn inspector_value_text(cfg: &Config, frame_index: u64, item_index: usize) -> String {
    let hot = item_is_hot(cfg, frame_index, item_index);
    match item_index & 3 {
        0 => {
            if hot {
                "Enabled".to_owned()
            } else if (item_index & 8) != 0 {
                "Disabled".to_owned()
            } else {
                "Enabled".to_owned()
            }
        }
        1 => {
            let base = 0.15 + ((item_index * 7) % 700) as f64 / 1000.0;
            let value = base + if hot { 0.125 } else { 0.0 };
            format!("{value:.3}")
        }
        2 => {
            let px = 8 + ((item_index * 13 + if hot { frame_index as usize } else { 0 }) % 512);
            format!("{px} px")
        }
        _ => {
            let color = fill_color(cfg, frame_index, item_index);
            format!(
                "#{:02X}{:02X}{:02X}",
                (color >> 24) & 0xff,
                (color >> 16) & 0xff,
                (color >> 8) & 0xff
            )
        }
    }
}

fn graph_has_right_link(cfg: &Config, item_index: usize) -> bool {
    let column = item_index % cfg.columns;
    column + 1 < cfg.columns && item_index + 1 < cfg.count
}

fn graph_has_down_link(cfg: &Config, item_index: usize) -> bool {
    item_index + cfg.columns < cfg.count
}

fn graph_has_diag_link(cfg: &Config, item_index: usize) -> bool {
    let column = item_index % cfg.columns;
    item_index % 3 == 0 && column + 1 < cfg.columns && item_index + cfg.columns + 1 < cfg.count
}

fn scene_init(cfg: &Config) -> Scene {
    let mut labels = Vec::with_capacity(cfg.count);
    let mut total_text_bytes = 0u64;
    for i in 0..cfg.count {
        let mut label = if cfg.scene_kind == SceneKind::Graph {
            format!("Node {i:05}")
        } else if cfg.scene_kind == SceneKind::Inspector {
            format!("Prop {i:05}")
        } else if cfg.scene_kind == SceneKind::Hierarchy {
            format!("Entry {i:05}")
        } else {
            format!("Item {i:05}")
        };
        label.truncate(LABEL_CAP);
        total_text_bytes += label.len() as u64;
        if cfg.scene_kind == SceneKind::Inspector {
            total_text_bytes += inspector_value_text(cfg, 0, i).len() as u64;
        } else if cfg.scene_kind == SceneKind::Hierarchy {
            total_text_bytes += hierarchy_kind_name(i).len() as u64;
        }
        labels.push(label);
    }
    Scene {
        labels,
        total_text_bytes,
    }
}

fn summary_print(library_name: &str, cfg: &Config, acc: &Accumulator) {
    let avg_frame_ms = average_or_zero(acc.total_frame_ms, acc.frames);
    let wall_fps = if acc.total_wall_seconds > 0.0 {
        acc.frames as f64 / acc.total_wall_seconds
    } else {
        0.0
    };
    println!(
        "COMPSUMMARY library={} mode={} scene={} scenario={} count={} mutate={} frames={} wall_fps={:.2} frame_ms={:.4} min_ms={:.4} max_ms={:.4} commands={} vertices={} indices={} text_bytes={}",
        library_name,
        cfg.mode_name,
        cfg.scene_name,
        cfg.scenario_name,
        cfg.count,
        cfg.mutate_count,
        acc.frames,
        wall_fps,
        avg_frame_ms,
        acc.min_frame_ms,
        acc.max_frame_ms,
        average_u64_or_zero(acc.total_command_count, acc.frames),
        average_u64_or_zero(acc.total_vertex_count, acc.frames),
        average_u64_or_zero(acc.total_index_count, acc.frames),
        average_u64_or_zero(acc.total_text_bytes, acc.frames)
    );
}

fn summary_append_csv(library_name: &str, cfg: &Config, acc: &Accumulator) {
    let Some(csv_path) = &cfg.csv_path else {
        return;
    };

    let write_header = !Path::new(csv_path).exists()
        || std::fs::metadata(csv_path).map(|m| m.len() == 0).unwrap_or(true);
    let mut out = OpenOptions::new()
        .create(true)
        .append(true)
        .open(csv_path)
        .unwrap_or_else(|e| {
            eprintln!("failed to open csv output: {csv_path}: {e}");
            std::process::exit(4);
        });

    if write_header {
        let _ = writeln!(
            out,
            "library,mode,scene,scenario,count,mutate_count,frames,wall_fps,avg_frame_ms,min_frame_ms,max_frame_ms,avg_commands,avg_vertices,avg_indices,avg_text_bytes"
        );
    }

    let avg_frame_ms = average_or_zero(acc.total_frame_ms, acc.frames);
    let wall_fps = if acc.total_wall_seconds > 0.0 {
        acc.frames as f64 / acc.total_wall_seconds
    } else {
        0.0
    };
    let _ = writeln!(
        out,
        "{},{},{},{},{},{},{},{:.4},{:.4},{:.4},{:.4},{},{},{},{}",
        library_name,
        cfg.mode_name,
        cfg.scene_name,
        cfg.scenario_name,
        cfg.count,
        cfg.mutate_count,
        acc.frames,
        wall_fps,
        avg_frame_ms,
        acc.min_frame_ms,
        acc.max_frame_ms,
        average_u64_or_zero(acc.total_command_count, acc.frames),
        average_u64_or_zero(acc.total_vertex_count, acc.frames),
        average_u64_or_zero(acc.total_index_count, acc.frames),
        average_u64_or_zero(acc.total_text_bytes, acc.frames)
    );
}

fn draw_graph_wire(painter: &egui::Painter, a: Pos2, b: Pos2, color: Color32) {
    let mid_x = a.x + (b.x - a.x) * 0.5;
    let elbow_a = Pos2::new(mid_x, a.y);
    let elbow_b = Pos2::new(mid_x, b.y);
    let stroke = Stroke::new(2.0, color);
    painter.line_segment([a, elbow_a], stroke);
    painter.line_segment([elbow_a, elbow_b], stroke);
    painter.line_segment([elbow_b, b], stroke);
}

fn render_cards(painter: &egui::Painter, cfg: &Config, scene: &Scene, frame_index: u64) {
    let font_id = FontId::proportional(14.0);
    for i in 0..cfg.count {
        let rect = scene_rect(cfg, i);
        painter.rect_filled(rect, CornerRadius::same(6), color32_from_u32(fill_color(cfg, frame_index, i)));
        painter.text(
            rect.min + Vec2::new(10.0, 9.0),
            Align2::LEFT_TOP,
            &scene.labels[i],
            font_id.clone(),
            color32_from_u32(text_color()),
        );
    }
}

fn render_text_wall(painter: &egui::Painter, cfg: &Config, scene: &Scene, frame_index: u64) {
    let font_id = FontId::proportional(14.0);
    for i in 0..cfg.count {
        let rect = scene_rect(cfg, i);
        painter.text(
            rect.min,
            Align2::LEFT_TOP,
            &scene.labels[i],
            font_id.clone(),
            color32_from_u32(if item_is_hot(cfg, frame_index, i) {
                hot_text_color()
            } else {
                text_color()
            }),
        );
    }
}

fn render_graph(painter: &egui::Painter, cfg: &Config, scene: &Scene, frame_index: u64) {
    let font_id = FontId::proportional(14.0);
    for i in 0..cfg.count {
        let rect = scene_rect(cfg, i);
        let a = Pos2::new(rect.max.x, rect.min.y + 40.0);

        if graph_has_right_link(cfg, i) {
            let target = scene_rect(cfg, i + 1);
            let hot = item_is_hot(cfg, frame_index, i) || item_is_hot(cfg, frame_index, i + 1);
            draw_graph_wire(
                painter,
                a,
                Pos2::new(target.min.x, target.min.y + 40.0),
                color32_from_u32(if hot { hot_wire_color() } else { wire_color() }),
            );
        }
        if graph_has_down_link(cfg, i) {
            let target = scene_rect(cfg, i + cfg.columns);
            let hot = item_is_hot(cfg, frame_index, i)
                || item_is_hot(cfg, frame_index, i + cfg.columns);
            draw_graph_wire(
                painter,
                a,
                Pos2::new(target.min.x, target.min.y + 40.0),
                color32_from_u32(if hot { hot_wire_color() } else { wire_color() }),
            );
        }
        if graph_has_diag_link(cfg, i) {
            let target = scene_rect(cfg, i + cfg.columns + 1);
            let hot = item_is_hot(cfg, frame_index, i)
                || item_is_hot(cfg, frame_index, i + cfg.columns + 1);
            draw_graph_wire(
                painter,
                a,
                Pos2::new(target.min.x, target.min.y + 40.0),
                color32_from_u32(if hot { hot_wire_color() } else { wire_color() }),
            );
        }
    }

    for i in 0..cfg.count {
        let rect = scene_rect(cfg, i);
        let hot = item_is_hot(cfg, frame_index, i);
        painter.rect_filled(
            rect,
            CornerRadius::same(8),
            color32_from_u32(fill_color(cfg, frame_index, i)),
        );
        let header = Rect::from_min_size(rect.min, Vec2::new(rect.width(), 24.0));
        painter.rect(
            header,
            CornerRadius::same(8),
            color32_from_u32(if hot { hot_wire_color() } else { rgba_u32(49, 55, 70, 255) }),
            Stroke::NONE,
            StrokeKind::Inside,
        );
        painter.text(
            rect.min + Vec2::new(10.0, 4.0),
            Align2::LEFT_TOP,
            &scene.labels[i],
            font_id.clone(),
            color32_from_u32(if hot { hot_text_color() } else { text_color() }),
        );
    }
}

fn render_inspector(painter: &egui::Painter, cfg: &Config, scene: &Scene, frame_index: u64) {
    let font_id = FontId::proportional(14.0);
    for i in 0..cfg.count {
        let rect = scene_rect(cfg, i);
        let hot = item_is_hot(cfg, frame_index, i);
        let value = inspector_value_text(cfg, frame_index, i);
        painter.rect_filled(
            rect,
            CornerRadius::same(5),
            color32_from_u32(fill_color(cfg, frame_index, i)),
        );
        painter.text(
            rect.min + Vec2::new(10.0, 5.0),
            Align2::LEFT_TOP,
            &scene.labels[i],
            font_id.clone(),
            color32_from_u32(if hot { hot_text_color() } else { text_color() }),
        );
        let pill = Rect::from_min_size(
            Pos2::new(rect.max.x - 126.0, rect.min.y + 3.0),
            Vec2::new(112.0, 18.0),
        );
        painter.rect_filled(
            pill,
            CornerRadius::same(4),
            color32_from_u32(inspector_value_fill_color(cfg, frame_index, i)),
        );
        painter.text(
            pill.min + Vec2::new(8.0, 2.0),
            Align2::LEFT_TOP,
            value,
            font_id.clone(),
            color32_from_u32(if hot { hot_text_color() } else { text_color() }),
        );
    }
}

fn render_hierarchy(painter: &egui::Painter, cfg: &Config, scene: &Scene, frame_index: u64) {
    let font_id = FontId::proportional(14.0);
    let chip_font = FontId::proportional(12.0);
    for i in 0..cfg.count {
        let rect = scene_rect(cfg, i);
        let depth = hierarchy_depth(i);
        let has_children = hierarchy_has_children(i);
        let indent = 12.0 + depth as f32 * 14.0;
        let hot = item_is_hot(cfg, frame_index, i);
        let marker = Rect::from_min_size(
            Pos2::new(rect.min.x + indent, rect.min.y + 5.0),
            Vec2::new(10.0, 10.0),
        );
        let chip = Rect::from_min_size(
            Pos2::new(rect.max.x - 58.0, rect.min.y + 2.0),
            Vec2::new(46.0, 16.0),
        );
        painter.rect_filled(
            rect,
            CornerRadius::same(4),
            color32_from_u32(hierarchy_row_fill_color(cfg, frame_index, i)),
        );
        painter.rect_filled(
            marker,
            CornerRadius::same(if has_children { 2 } else { 5 }),
            color32_from_u32(hierarchy_marker_fill_color(cfg, frame_index, i)),
        );
        painter.rect_filled(
            chip,
            CornerRadius::same(4),
            color32_from_u32(hierarchy_kind_fill_color(i)),
        );
        painter.text(
            rect.min + Vec2::new(indent + 18.0, 3.0),
            Align2::LEFT_TOP,
            &scene.labels[i],
            font_id.clone(),
            color32_from_u32(if hot { hot_text_color() } else { text_color() }),
        );
        painter.text(
            chip.min + Vec2::new(6.0, 2.0),
            Align2::LEFT_TOP,
            hierarchy_kind_name(i),
            chip_font.clone(),
            color32_from_u32(if hot { hot_text_color() } else { text_color() }),
        );
    }
}

fn render_frame(ctx: &Context, cfg: &Config, scene: &Scene, frame_index: u64) -> FrameMetrics {
    let raw_input = RawInput {
        screen_rect: Some(Rect::from_min_size(
            Pos2::ZERO,
            Vec2::new(cfg.width as f32, cfg.height as f32),
        )),
        time: Some(frame_index as f64 * (1.0 / 60.0)),
        predicted_dt: 1.0 / 60.0,
        ..Default::default()
    };

    let layer_id = LayerId::new(Order::Background, Id::new("bench"));
    let full_output = ctx.run(raw_input, |ctx| {
        let painter = ctx.layer_painter(layer_id);
        match cfg.scene_kind {
            SceneKind::Cards => render_cards(&painter, cfg, scene, frame_index),
            SceneKind::TextWall => render_text_wall(&painter, cfg, scene, frame_index),
            SceneKind::Graph => render_graph(&painter, cfg, scene, frame_index),
            SceneKind::Inspector => render_inspector(&painter, cfg, scene, frame_index),
            SceneKind::Hierarchy => render_hierarchy(&painter, cfg, scene, frame_index),
        }
    });

    let clipped_primitives = ctx.tessellate(full_output.shapes, full_output.pixels_per_point);
    let mut vertex_count = 0u64;
    let mut index_count = 0u64;
    for clipped in &clipped_primitives {
        if let egui::epaint::Primitive::Mesh(mesh) = &clipped.primitive {
            vertex_count += mesh.vertices.len() as u64;
            index_count += mesh.indices.len() as u64;
        }
    }

    FrameMetrics {
        frame_ms: 0.0,
        command_count: clipped_primitives.len() as u64,
        vertex_count,
        index_count,
        text_bytes: scene.total_text_bytes,
    }
}

fn main() {
    let cfg = parse_args();
    let scene = scene_init(&cfg);
    let mut acc = accum_reset();
    let ctx = Context::default();

    let start = Instant::now();
    let warmup_end = start + std::time::Duration::from_secs_f64(cfg.warmup_seconds);
    let end = warmup_end + std::time::Duration::from_secs_f64(cfg.seconds);
    let mut frame_index = 0u64;

    while Instant::now() < end {
        let frame_start = Instant::now();
        let mut frame = render_frame(&ctx, &cfg, &scene, frame_index);
        let frame_end = Instant::now();
        frame.frame_ms = (frame_end - frame_start).as_secs_f64() * 1000.0;

        if frame_end >= warmup_end {
            accum_add(&mut acc, &frame);
            acc.total_wall_seconds += (frame_end - frame_start).as_secs_f64();
        }
        frame_index += 1;
    }

    summary_print("egui", &cfg, &acc);
    summary_append_csv("egui", &cfg, &acc);
}
