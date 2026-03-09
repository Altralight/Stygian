const std = @import("std");
const dvui = @import("dvui");

const LABEL_CAP = 32;
const LibraryName = "dvui";
const DefaultModeName = "headless-primitive";

const Scenario = enum {
    static,
    sparse,
    fullhot,
};

const SceneKind = enum {
    cards,
    textwall,
    graph,
    inspector,
    hierarchy,
};

const Config = struct {
    width: i32 = 1920,
    height: i32 = 1080,
    count: usize = 10_000,
    mutate_count: usize = 100,
    columns: usize = 16,
    seconds: f64 = 2.0,
    warmup_seconds: f64 = 1.0,
    cell_width: f32 = 112.0,
    cell_height: f32 = 34.0,
    gap: f32 = 6.0,
    csv_path: ?[]const u8 = null,
    mode_name: []const u8 = DefaultModeName,
    scene_kind: SceneKind = .cards,
    scenario: Scenario = .static,
    owns_csv_path: bool = false,
    owns_mode_name: bool = false,

    fn sceneName(self: Config) []const u8 {
        return switch (self.scene_kind) {
            .cards => "cards",
            .textwall => "textwall",
            .graph => "graph",
            .inspector => "inspector",
            .hierarchy => "hierarchy",
        };
    }

    fn scenarioName(self: Config) []const u8 {
        return switch (self.scenario) {
            .static => "static",
            .sparse => "sparse",
            .fullhot => "fullhot",
        };
    }
};

const FrameMetrics = struct {
    frame_ms: f64,
    command_count: u64,
    vertex_count: u64,
    index_count: u64,
    text_bytes: u64,
};

const Accumulator = struct {
    frames: u64 = 0,
    total_frame_ms: f64 = 0,
    min_frame_ms: f64 = std.math.floatMax(f64),
    max_frame_ms: f64 = 0,
    total_wall_seconds: f64 = 0,
    total_command_count: u64 = 0,
    total_vertex_count: u64 = 0,
    total_index_count: u64 = 0,
    total_text_bytes: u64 = 0,

    fn add(self: *Accumulator, frame: FrameMetrics) void {
        self.frames += 1;
        self.total_frame_ms += frame.frame_ms;
        self.min_frame_ms = @min(self.min_frame_ms, frame.frame_ms);
        self.max_frame_ms = @max(self.max_frame_ms, frame.frame_ms);
        self.total_command_count += frame.command_count;
        self.total_vertex_count += frame.vertex_count;
        self.total_index_count += frame.index_count;
        self.total_text_bytes += frame.text_bytes;
    }
};

const Label = struct {
    buf: [LABEL_CAP]u8 = [_]u8{0} ** LABEL_CAP,
    len: u8 = 0,

    fn slice(self: *const Label) []const u8 {
        return self.buf[0..self.len];
    }
};

const Scene = struct {
    labels: []Label,
    total_text_bytes: u64,

    fn init(allocator: std.mem.Allocator, cfg: Config) !Scene {
        const labels = try allocator.alloc(Label, cfg.count);
        var total_text_bytes: u64 = 0;

        for (labels, 0..) |*label, i| {
            const text = if (cfg.scene_kind == .graph)
                try std.fmt.bufPrint(&label.buf, "Node {d:0>5}", .{i})
            else if (cfg.scene_kind == .inspector)
                try std.fmt.bufPrint(&label.buf, "Prop {d:0>5}", .{i})
            else if (cfg.scene_kind == .hierarchy)
                try std.fmt.bufPrint(&label.buf, "Entry {d:0>5}", .{i})
            else
                try std.fmt.bufPrint(&label.buf, "Item {d:0>5}", .{i});
            label.len = @intCast(text.len);
            total_text_bytes += text.len;
            if (cfg.scene_kind == .inspector) {
                var value_buf: [LABEL_CAP]u8 = undefined;
                total_text_bytes += inspectorValueText(&value_buf, cfg, 0, i).len;
            } else if (cfg.scene_kind == .hierarchy) {
                total_text_bytes += hierarchyKindName(i).len;
            }
        }

        return .{
            .labels = labels,
            .total_text_bytes = total_text_bytes,
        };
    }

    fn deinit(self: *Scene, allocator: std.mem.Allocator) void {
        allocator.free(self.labels);
        self.* = undefined;
    }

    fn labelAt(self: *const Scene, index: usize) []const u8 {
        return self.labels[index].slice();
    }
};

const Harness = struct {
    cfg: Config,
    scene: Scene,
    frame_index: u64 = 0,

    fn draw(self: *Harness) void {
        dvui.windowRectPixels().fill(.{}, .{ .color = rgba(13, 15, 18, 255), .fade = 1.0 });

        switch (self.cfg.scene_kind) {
            .cards => self.drawCards(),
            .textwall => self.drawTextWall(),
            .graph => self.drawGraph(),
            .inspector => self.drawInspector(),
            .hierarchy => self.drawHierarchy(),
        }
    }

    fn drawCards(self: *Harness) void {
        for (0..self.cfg.count) |i| {
            const rect = sceneRect(self.cfg, i);
            fillRounded(rect, 6.0, fillColor(self.cfg, self.frame_index, i));
            drawLabel(i, self.scene.labelAt(i), .{
                .x = rect.x + 10.0,
                .y = rect.y + 7.0,
                .w = rect.w - 20.0,
                .h = rect.h - 14.0,
            }, textColorFor(self.cfg, self.frame_index, i));
        }
    }

    fn drawTextWall(self: *Harness) void {
        for (0..self.cfg.count) |i| {
            const rect = sceneRect(self.cfg, i);
            drawLabel(i, self.scene.labelAt(i), rect, textColorFor(self.cfg, self.frame_index, i));
        }
    }

    fn drawGraph(self: *Harness) void {
        for (0..self.cfg.count) |i| {
            const rect = sceneRect(self.cfg, i);
            const source_pin = pointPhysical(rect.x + rect.w, rect.y + 39.0);

            if (graphHasRightLink(self.cfg, i)) {
                const target = sceneRect(self.cfg, i + 1);
                const hot = itemIsHot(self.cfg, self.frame_index, i) or itemIsHot(self.cfg, self.frame_index, i + 1);
                drawWire(source_pin, pointPhysical(target.x, target.y + 39.0), if (hot) hotWireColor() else wireColor());
            }
            if (graphHasDownLink(self.cfg, i)) {
                const target = sceneRect(self.cfg, i + self.cfg.columns);
                const hot = itemIsHot(self.cfg, self.frame_index, i) or itemIsHot(self.cfg, self.frame_index, i + self.cfg.columns);
                drawWire(source_pin, pointPhysical(target.x, target.y + 39.0), if (hot) hotWireColor() else wireColor());
            }
            if (graphHasDiagLink(self.cfg, i)) {
                const target = sceneRect(self.cfg, i + self.cfg.columns + 1);
                const hot = itemIsHot(self.cfg, self.frame_index, i) or itemIsHot(self.cfg, self.frame_index, i + self.cfg.columns + 1);
                drawWire(source_pin, pointPhysical(target.x, target.y + 39.0), if (hot) hotWireColor() else wireColor());
            }
        }

        for (0..self.cfg.count) |i| {
            const rect = sceneRect(self.cfg, i);
            const fill = fillColor(self.cfg, self.frame_index, i);
            fillRounded(rect, 8.0, fill);
            fillRounded(.{
                .x = rect.x,
                .y = rect.y,
                .w = rect.w,
                .h = 24.0,
            }, 8.0, if (itemIsHot(self.cfg, self.frame_index, i)) hotWireColor() else rgba(49, 55, 70, 255));
            drawLabel(i, self.scene.labelAt(i), .{
                .x = rect.x + 10.0,
                .y = rect.y + 4.0,
                .w = rect.w - 20.0,
                .h = 18.0,
            }, textColorFor(self.cfg, self.frame_index, i));
        }
    }

    fn drawInspector(self: *Harness) void {
        for (0..self.cfg.count) |i| {
            var value_buf: [LABEL_CAP]u8 = undefined;
            const rect = sceneRect(self.cfg, i);
            const hot = itemIsHot(self.cfg, self.frame_index, i);
            const value = inspectorValueText(&value_buf, self.cfg, self.frame_index, i);
            fillRounded(rect, 5.0, fillColor(self.cfg, self.frame_index, i));
            drawLabel(i, self.scene.labelAt(i), .{
                .x = rect.x + 10.0,
                .y = rect.y + 5.0,
                .w = 180.0,
                .h = rect.h - 8.0,
            }, if (hot) hotTextColor() else textColor());
            fillRounded(.{
                .x = rect.x + rect.w - 126.0,
                .y = rect.y + 3.0,
                .w = 112.0,
                .h = 18.0,
            }, 4.0, inspectorValueFillColor(self.cfg, self.frame_index, i));
            drawLabel(i + self.cfg.count, value, .{
                .x = rect.x + rect.w - 118.0,
                .y = rect.y + 5.0,
                .w = 104.0,
                .h = 14.0,
            }, if (hot) hotTextColor() else textColor());
        }
    }

    fn drawHierarchy(self: *Harness) void {
        for (0..self.cfg.count) |i| {
            const rect = sceneRect(self.cfg, i);
            const depth = hierarchyDepth(i);
            const has_children = hierarchyHasChildren(i);
            const indent = 12.0 + @as(f32, @floatFromInt(depth)) * 14.0;
            const hot = itemIsHot(self.cfg, self.frame_index, i);
            fillRounded(rect, 4.0, hierarchyRowFillColor(self.cfg, self.frame_index, i));
            fillRounded(.{
                .x = rect.x + indent,
                .y = rect.y + 5.0,
                .w = 10.0,
                .h = 10.0,
            }, if (has_children) 2.0 else 5.0, hierarchyMarkerFillColor(self.cfg, self.frame_index, i));
            fillRounded(.{
                .x = rect.x + rect.w - 58.0,
                .y = rect.y + 2.0,
                .w = 46.0,
                .h = 16.0,
            }, 4.0, hierarchyKindFillColor(i));
            drawLabel(i, self.scene.labelAt(i), .{
                .x = rect.x + indent + 18.0,
                .y = rect.y + 3.0,
                .w = rect.w - indent - 84.0,
                .h = 14.0,
            }, if (hot) hotTextColor() else textColor());
            drawLabel(i + self.cfg.count, hierarchyKindName(i), .{
                .x = rect.x + rect.w - 50.0,
                .y = rect.y + 3.0,
                .w = 38.0,
                .h = 12.0,
            }, if (hot) hotTextColor() else textColor());
        }
    }
};

pub fn main() !void {
    var gpa_state = std.heap.GeneralPurposeAllocator(.{}){};
    defer {
        const leaked = gpa_state.deinit();
        if (leaked != .ok) {
            std.debug.print("leaked memory in dvui benchmark\n", .{});
        }
    }
    const gpa = gpa_state.allocator();

    var cfg = try parseArgs(gpa);
    defer deinitConfig(gpa, &cfg);
    var scene = try Scene.init(gpa, cfg);
    defer scene.deinit(gpa);

    var harness = Harness{
        .cfg = cfg,
        .scene = scene,
    };

    var t = try dvui.testing.init(.{
        .allocator = gpa,
        .window_size = .{
            .w = @floatFromInt(cfg.width),
            .h = @floatFromInt(cfg.height),
        },
    });
    defer t.deinit();

    var warmup_timer = try std.time.Timer.start();
    while (secondsSince(&warmup_timer) < cfg.warmup_seconds) {
        _ = try runFrame(&t, &harness);
    }

    var acc = Accumulator{};
    var measure_timer = try std.time.Timer.start();
    while (secondsSince(&measure_timer) < cfg.seconds) {
        acc.add(try runFrame(&t, &harness));
    }
    acc.total_wall_seconds = secondsSince(&measure_timer);

    printSummary(cfg, acc);
    try appendCsv(cfg, acc);
}

fn parseArgs(allocator: std.mem.Allocator) !Config {
    var cfg = Config{};
    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    var i: usize = 1;
    while (i < args.len) {
        const arg = args[i];
        if (std.mem.eql(u8, arg, "--scene") and i + 1 < args.len) {
            cfg.scene_kind = parseSceneKind(args[i + 1]);
            i += 2;
            continue;
        }
        if (std.mem.eql(u8, arg, "--scenario") and i + 1 < args.len) {
            cfg.scenario = parseScenario(args[i + 1]);
            i += 2;
            continue;
        }
        if (std.mem.eql(u8, arg, "--count") and i + 1 < args.len) {
            cfg.count = try std.fmt.parseInt(usize, args[i + 1], 10);
            i += 2;
            continue;
        }
        if (std.mem.eql(u8, arg, "--mutate-count") and i + 1 < args.len) {
            cfg.mutate_count = try std.fmt.parseInt(usize, args[i + 1], 10);
            i += 2;
            continue;
        }
        if (std.mem.eql(u8, arg, "--seconds") and i + 1 < args.len) {
            cfg.seconds = try std.fmt.parseFloat(f64, args[i + 1]);
            i += 2;
            continue;
        }
        if (std.mem.eql(u8, arg, "--warmup-seconds") and i + 1 < args.len) {
            cfg.warmup_seconds = try std.fmt.parseFloat(f64, args[i + 1]);
            i += 2;
            continue;
        }
        if (std.mem.eql(u8, arg, "--columns") and i + 1 < args.len) {
            cfg.columns = try std.fmt.parseInt(usize, args[i + 1], 10);
            i += 2;
            continue;
        }
        if (std.mem.eql(u8, arg, "--width") and i + 1 < args.len) {
            cfg.width = try std.fmt.parseInt(i32, args[i + 1], 10);
            i += 2;
            continue;
        }
        if (std.mem.eql(u8, arg, "--height") and i + 1 < args.len) {
            cfg.height = try std.fmt.parseInt(i32, args[i + 1], 10);
            i += 2;
            continue;
        }
        if (std.mem.eql(u8, arg, "--csv") and i + 1 < args.len) {
            cfg.csv_path = try allocator.dupe(u8, args[i + 1]);
            cfg.owns_csv_path = true;
            i += 2;
            continue;
        }
        if (std.mem.eql(u8, arg, "--mode") and i + 1 < args.len) {
            cfg.mode_name = try allocator.dupe(u8, args[i + 1]);
            cfg.owns_mode_name = true;
            i += 2;
            continue;
        }

        usage(args[0]);
        return error.InvalidArguments;
    }

    if (cfg.count == 0 or cfg.columns == 0 or cfg.seconds <= 0.0 or cfg.warmup_seconds < 0.0) {
        std.debug.print("bad benchmark config\n", .{});
        return error.InvalidArguments;
    }
    if (cfg.mutate_count > cfg.count) {
        cfg.mutate_count = cfg.count;
    }
    return cfg;
}

fn deinitConfig(allocator: std.mem.Allocator, cfg: *Config) void {
    if (cfg.owns_csv_path and cfg.csv_path) |csv_path| {
        allocator.free(csv_path);
    }
    if (cfg.owns_mode_name) {
        allocator.free(cfg.mode_name);
    }
}

fn usage(program_name: []const u8) void {
    std.debug.print(
        "usage: {s} [--scene cards|textwall|graph|inspector|hierarchy] [--scenario static|sparse|fullhot] [--count N] [--mutate-count N]\n",
        .{program_name},
    );
    std.debug.print(
        "          [--seconds S] [--warmup-seconds S] [--columns N] [--width W] [--height H] [--csv path]\n",
        .{},
    );
}

fn parseSceneKind(value: []const u8) SceneKind {
    if (std.mem.eql(u8, value, "cards")) return .cards;
    if (std.mem.eql(u8, value, "textwall")) return .textwall;
    if (std.mem.eql(u8, value, "graph")) return .graph;
    if (std.mem.eql(u8, value, "inspector")) return .inspector;
    if (std.mem.eql(u8, value, "hierarchy")) return .hierarchy;
    std.debug.print("unknown scene: {s}\n", .{value});
    std.process.exit(2);
}

fn parseScenario(value: []const u8) Scenario {
    if (std.mem.eql(u8, value, "static")) return .static;
    if (std.mem.eql(u8, value, "sparse")) return .sparse;
    if (std.mem.eql(u8, value, "fullhot")) return .fullhot;
    std.debug.print("unknown scenario: {s}\n", .{value});
    std.process.exit(2);
}

fn runFrame(t: anytype, harness: *Harness) !FrameMetrics {
    t.backend.statsReset();

    var timer = try std.time.Timer.start();
    harness.draw();
    _ = try t.window.end(.{});
    const backend_stats = t.backend.statsSnapshot();
    try t.window.begin(t.window.frame_time_ns + 100 * std.time.ns_per_ms);

    harness.frame_index += 1;

    return .{
        .frame_ms = @as(f64, @floatFromInt(timer.read())) / 1_000_000.0,
        .command_count = backend_stats.draw_calls,
        .vertex_count = backend_stats.vertices,
        .index_count = backend_stats.indices,
        .text_bytes = harness.scene.total_text_bytes,
    };
}

fn secondsSince(timer: *std.time.Timer) f64 {
    return @as(f64, @floatFromInt(timer.read())) / @as(f64, @floatFromInt(std.time.ns_per_s));
}

fn averageOrZero(total: f64, count: u64) f64 {
    if (count == 0) return 0.0;
    return total / @as(f64, @floatFromInt(count));
}

fn averageU64OrZero(total: u64, count: u64) u64 {
    if (count == 0) return 0;
    return total / count;
}

fn printSummary(cfg: Config, acc: Accumulator) void {
    const avg_frame_ms = averageOrZero(acc.total_frame_ms, acc.frames);
    const wall_fps = if (acc.total_wall_seconds > 0.0)
        @as(f64, @floatFromInt(acc.frames)) / acc.total_wall_seconds
    else
        0.0;

    std.debug.print(
        "COMPSUMMARY library={s} mode={s} scene={s} scenario={s} count={d} mutate={d} frames={d} wall_fps={d:.2} frame_ms={d:.4} min_ms={d:.4} max_ms={d:.4} commands={d} vertices={d} indices={d} text_bytes={d}\n",
        .{
            LibraryName,
            cfg.mode_name,
            cfg.sceneName(),
            cfg.scenarioName(),
            cfg.count,
            cfg.mutate_count,
            acc.frames,
            wall_fps,
            avg_frame_ms,
            if (acc.frames == 0) 0.0 else acc.min_frame_ms,
            acc.max_frame_ms,
            averageU64OrZero(acc.total_command_count, acc.frames),
            averageU64OrZero(acc.total_vertex_count, acc.frames),
            averageU64OrZero(acc.total_index_count, acc.frames),
            averageU64OrZero(acc.total_text_bytes, acc.frames),
        },
    );
}

fn appendCsv(cfg: Config, acc: Accumulator) !void {
    const csv_path = cfg.csv_path orelse return;

    var file = std.fs.cwd().openFile(csv_path, .{ .mode = .read_write }) catch |err| switch (err) {
        error.FileNotFound => try std.fs.cwd().createFile(csv_path, .{ .read = true, .truncate = false }),
        else => return err,
    };
    defer file.close();

    const write_header = (try file.getEndPos()) == 0;
    try file.seekFromEnd(0);

    var writer = file.writer();

    if (write_header) {
        try writer.print(
            "library,mode,scene,scenario,count,mutate_count,frames,wall_fps,avg_frame_ms,min_frame_ms,max_frame_ms,avg_commands,avg_vertices,avg_indices,avg_text_bytes\n",
            .{},
        );
    }

    const avg_frame_ms = averageOrZero(acc.total_frame_ms, acc.frames);
    const wall_fps = if (acc.total_wall_seconds > 0.0)
        @as(f64, @floatFromInt(acc.frames)) / acc.total_wall_seconds
    else
        0.0;

    try writer.print(
        "{s},{s},{s},{s},{d},{d},{d},{d:.4},{d:.4},{d:.4},{d:.4},{d},{d},{d},{d}\n",
        .{
            LibraryName,
            cfg.mode_name,
            cfg.sceneName(),
            cfg.scenarioName(),
            cfg.count,
            cfg.mutate_count,
            acc.frames,
            wall_fps,
            avg_frame_ms,
            if (acc.frames == 0) 0.0 else acc.min_frame_ms,
            acc.max_frame_ms,
            averageU64OrZero(acc.total_command_count, acc.frames),
            averageU64OrZero(acc.total_vertex_count, acc.frames),
            averageU64OrZero(acc.total_index_count, acc.frames),
            averageU64OrZero(acc.total_text_bytes, acc.frames),
        },
    );
    try file.sync();
}

fn itemIsHot(cfg: Config, frame_index: u64, item_index: usize) bool {
    if (cfg.scenario == .fullhot) return true;
    if (cfg.scenario != .sparse or cfg.mutate_count == 0) return false;

    const start = @as(usize, @intCast((frame_index * cfg.mutate_count) % cfg.count));
    const end = start + cfg.mutate_count;
    if (end <= cfg.count) {
        return item_index >= start and item_index < end;
    }
    return item_index >= start or item_index < (end - cfg.count);
}

fn fillColor(cfg: Config, frame_index: u64, item_index: usize) dvui.Color {
    var r: u8 = @intCast(26 + (item_index % 5) * 8);
    var g: u8 = @intCast(31 + (item_index % 7) * 6);
    var b: u8 = @intCast(39 + (item_index % 11) * 5);

    if (itemIsHot(cfg, frame_index, item_index)) {
        r = @intCast(60 + ((frame_index + item_index * 3) % 120));
        g = @intCast(110 + ((frame_index + item_index * 5) % 100));
        b = @intCast(180 + ((frame_index + item_index * 7) % 60));
    }

    return rgba(r, g, b, 255);
}

fn textColorFor(cfg: Config, frame_index: u64, item_index: usize) dvui.Color {
    return if (itemIsHot(cfg, frame_index, item_index))
        hotTextColor()
    else
        textColor();
}

fn textColor() dvui.Color {
    return rgba(230, 233, 240, 255);
}

fn hotTextColor() dvui.Color {
    return rgba(163, 214, 255, 255);
}

fn wireColor() dvui.Color {
    return rgba(74, 158, 255, 220);
}

fn hotWireColor() dvui.Color {
    return rgba(110, 210, 255, 255);
}

fn inspectorValueFillColor(cfg: Config, frame_index: u64, item_index: usize) dvui.Color {
    return if (itemIsHot(cfg, frame_index, item_index))
        hotWireColor()
    else
        rgba(49, 55, 70, 255);
}

fn rgba(r: u8, g: u8, b: u8, a: u8) dvui.Color {
    return .{ .r = r, .g = g, .b = b, .a = a };
}

fn sceneRect(cfg: Config, item_index: usize) dvui.Rect {
    return switch (cfg.scene_kind) {
        .cards => cellRect(cfg, item_index),
        .textwall => textLineRect(cfg, item_index),
        .graph => graphNodeRect(cfg, item_index),
        .inspector => inspectorRowRect(cfg, item_index),
        .hierarchy => hierarchyRowRect(cfg, item_index),
    };
}

fn cellRect(cfg: Config, item_index: usize) dvui.Rect {
    const row = item_index / cfg.columns;
    const column = item_index % cfg.columns;
    return .{
        .x = cfg.gap + @as(f32, @floatFromInt(column)) * (cfg.cell_width + cfg.gap),
        .y = cfg.gap + @as(f32, @floatFromInt(row)) * (cfg.cell_height + cfg.gap),
        .w = cfg.cell_width,
        .h = cfg.cell_height,
    };
}

fn textLineRect(cfg: Config, item_index: usize) dvui.Rect {
    const line_height = 18.0;
    const column_width = 248.0;
    const row = item_index / cfg.columns;
    const column = item_index % cfg.columns;
    return .{
        .x = 14.0 + @as(f32, @floatFromInt(column)) * (column_width + 24.0),
        .y = 14.0 + @as(f32, @floatFromInt(row)) * line_height,
        .w = column_width,
        .h = line_height,
    };
}

fn graphNodeRect(cfg: Config, item_index: usize) dvui.Rect {
    const node_width = 136.0;
    const node_height = 78.0;
    const pitch_x = 172.0;
    const pitch_y = 112.0;
    const row = item_index / cfg.columns;
    const column = item_index % cfg.columns;
    const stagger: f32 = if ((row & 1) == 1) 18.0 else -18.0;
    return .{
        .x = 28.0 + @as(f32, @floatFromInt(column)) * pitch_x + stagger,
        .y = 24.0 + @as(f32, @floatFromInt(row)) * pitch_y + @as(f32, @floatFromInt((column % 3) * 7)),
        .w = node_width,
        .h = node_height,
    };
}

fn inspectorRowRect(cfg: Config, item_index: usize) dvui.Rect {
    const columns = std.math.clamp(cfg.columns, 1, 4);
    const row_height = 28.0;
    const pane_width = 360.0;
    const pane_gap = 32.0;
    const row = item_index / columns;
    const column = item_index % columns;
    return .{
        .x = 20.0 + @as(f32, @floatFromInt(column)) * (pane_width + pane_gap),
        .y = 18.0 + @as(f32, @floatFromInt(row)) * row_height,
        .w = pane_width,
        .h = 24.0,
    };
}

fn hierarchyRowRect(cfg: Config, item_index: usize) dvui.Rect {
    const columns = std.math.clamp(cfg.columns, 1, 3);
    const row_height = 24.0;
    const pane_width = 320.0;
    const pane_gap = 26.0;
    const row = item_index / columns;
    const column = item_index % columns;
    return .{
        .x = 18.0 + @as(f32, @floatFromInt(column)) * (pane_width + pane_gap),
        .y = 18.0 + @as(f32, @floatFromInt(row)) * row_height,
        .w = pane_width,
        .h = 20.0,
    };
}

fn hierarchyDepth(item_index: usize) usize {
    var depth: usize = 0;
    var cursor = item_index;
    while (cursor % 7 != 0 and depth < 4) {
        depth += 1;
        cursor /= 2;
    }
    return depth;
}

fn hierarchyHasChildren(item_index: usize) bool {
    return item_index % 5 != 4;
}

fn hierarchyRowFillColor(cfg: Config, frame_index: u64, item_index: usize) dvui.Color {
    if (itemIsHot(cfg, frame_index, item_index)) return rgba(38, 66, 103, 255);
    if (item_index % 6 == 0) return rgba(28, 33, 42, 255);
    return rgba(22, 26, 33, 220);
}

fn hierarchyMarkerFillColor(cfg: Config, frame_index: u64, item_index: usize) dvui.Color {
    return if (itemIsHot(cfg, frame_index, item_index)) hotWireColor() else rgba(74, 86, 104, 255);
}

fn hierarchyKindFillColor(item_index: usize) dvui.Color {
    return switch (item_index % 4) {
        0 => rgba(49, 55, 70, 255),
        1 => rgba(58, 70, 92, 255),
        2 => rgba(61, 79, 63, 255),
        else => rgba(82, 64, 45, 255),
    };
}

fn hierarchyKindName(item_index: usize) []const u8 {
    return switch (item_index % 4) {
        0 => "Mesh",
        1 => "Light",
        2 => "Group",
        else => "Cam",
    };
}

fn inspectorValueText(buffer: *[LABEL_CAP]u8, cfg: Config, frame_index: u64, item_index: usize) []const u8 {
    const hot = itemIsHot(cfg, frame_index, item_index);
    return switch (item_index & 3) {
        0 => std.fmt.bufPrint(buffer, "{s}", .{
            if (hot) "Enabled" else if ((item_index & 8) != 0) "Disabled" else "Enabled",
        }) catch "Enabled",
        1 => std.fmt.bufPrint(buffer, "{d:.3}", .{
            0.15 + @as(f64, @floatFromInt((item_index * 7) % 700)) / 1000.0 + if (hot) 0.125 else 0.0,
        }) catch "0.000",
        2 => std.fmt.bufPrint(buffer, "{d} px", .{
            8 + ((item_index * 13 + if (hot) @as(usize, @intCast(frame_index)) else 0) % 512),
        }) catch "0 px",
        else => blk: {
            const color = fillColor(cfg, frame_index, item_index);
            break :blk std.fmt.bufPrint(buffer, "#{X:0>2}{X:0>2}{X:0>2}", .{
                color.r,
                color.g,
                color.b,
            }) catch "#000000";
        },
    };
}

fn graphHasRightLink(cfg: Config, item_index: usize) bool {
    const column = item_index % cfg.columns;
    return (column + 1 < cfg.columns) and (item_index + 1 < cfg.count);
}

fn graphHasDownLink(cfg: Config, item_index: usize) bool {
    return (item_index + cfg.columns) < cfg.count;
}

fn graphHasDiagLink(cfg: Config, item_index: usize) bool {
    const column = item_index % cfg.columns;
    return ((item_index % 3) == 0) and (column + 1 < cfg.columns) and (item_index + cfg.columns + 1 < cfg.count);
}

fn fillRounded(rect: dvui.Rect, radius: f32, color: dvui.Color) void {
    const scale = dvui.windowNaturalScale();
    rect.scale(scale, dvui.Rect.Physical).fill(dvui.Rect.Physical.all(radius * scale), .{
        .color = color,
        .fade = 1.0,
    });
}

fn drawLabel(index: usize, text: []const u8, rect: dvui.Rect, color: dvui.Color) void {
    dvui.labelNoFmt(@src(), text, .{}, .{
        .id_extra = index,
        .rect = rect,
        .background = false,
        .color_text = color,
        .padding = dvui.Rect.all(0),
        .margin = dvui.Rect.all(0),
    });
}

fn pointPhysical(x: f32, y: f32) dvui.Point.Physical {
    const scale = dvui.windowNaturalScale();
    return .{ .x = x * scale, .y = y * scale };
}

fn drawWire(from: dvui.Point.Physical, to: dvui.Point.Physical, color: dvui.Color) void {
    const mid_x = from.x + (to.x - from.x) * 0.5;
    const points = [_]dvui.Point.Physical{
        from,
        .{ .x = mid_x, .y = from.y },
        .{ .x = mid_x, .y = to.y },
        to,
    };
    const path = dvui.Path{ .points = &points };
    path.stroke(.{
        .thickness = 2.0 * dvui.windowNaturalScale(),
        .color = color,
        .endcap_style = .square,
    });
}
