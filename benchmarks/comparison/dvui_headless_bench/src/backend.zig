const std = @import("std");
const dvui = @import("dvui");

pub const kind: dvui.enums.Backend = .testing;

pub const Stats = struct {
    draw_calls: u64 = 0,
    vertices: u64 = 0,
    indices: u64 = 0,
};

pub const TestingBackend = @This();
pub const Context = *TestingBackend;

allocator: std.mem.Allocator,
arena: std.mem.Allocator = undefined,
size: dvui.Size.Natural,
size_pixels: dvui.Size.Physical,
time: i128 = 0,
clipboard: ?[]const u8 = null,
cursor_shown: bool = true,
stats: Stats = .{},

pub const InitOptions = struct {
    allocator: std.mem.Allocator,
    size: dvui.Size.Natural,
    size_pixels: dvui.Size.Physical,
};

pub fn init(opts: InitOptions) TestingBackend {
    return .{
        .allocator = opts.allocator,
        .size = opts.size,
        .size_pixels = opts.size_pixels,
    };
}

pub fn deinit(self: *TestingBackend) void {
    if (self.clipboard) |text| {
        self.allocator.free(text);
    }
    self.* = undefined;
}

pub fn statsReset(self: *TestingBackend) void {
    self.stats = .{};
}

pub fn statsSnapshot(self: *const TestingBackend) Stats {
    return self.stats;
}

pub fn nanoTime(self: *TestingBackend) i128 {
    defer self.time += 1 * std.time.ns_per_ms;
    return self.time;
}

pub fn sleep(_: *TestingBackend, _: u64) void {}

pub fn begin(self: *TestingBackend, arena: std.mem.Allocator) !void {
    self.arena = arena;
}

pub fn end(_: *TestingBackend) !void {}

pub fn pixelSize(self: *TestingBackend) dvui.Size.Physical {
    return self.size_pixels;
}

pub fn windowSize(self: *TestingBackend) dvui.Size.Natural {
    return self.size;
}

pub fn contentScale(_: *TestingBackend) f32 {
    return 1;
}

pub fn drawClippedTriangles(self: *TestingBackend, _: ?dvui.Texture, vtx: []const dvui.Vertex, idx: []const u16, _: ?dvui.Rect.Physical) !void {
    self.stats.draw_calls += 1;
    self.stats.vertices += vtx.len;
    self.stats.indices += idx.len;
}

pub fn textureCreate(self: *TestingBackend, pixels: [*]const u8, width: u32, height: u32, _: dvui.enums.TextureInterpolation) !dvui.Texture {
    const new_pixels = self.allocator.dupe(u8, pixels[0 .. width * height * 4]) catch @panic("Couldn't create texture: OOM");
    return .{
        .width = width,
        .height = height,
        .ptr = new_pixels.ptr,
    };
}

pub fn textureCreateTarget(_: *TestingBackend, _: u32, _: u32, _: dvui.enums.TextureInterpolation) !dvui.TextureTarget {
    return error.TextureCreate;
}

pub fn textureReadTarget(_: *TestingBackend, texture: dvui.TextureTarget, pixels: [*]u8) !void {
    const ptr: [*]const u8 = @ptrCast(texture.ptr);
    @memcpy(pixels, ptr[0 .. (texture.width * texture.height * 4)]);
}

pub fn textureDestroy(self: *TestingBackend, texture: dvui.Texture) void {
    const ptr: [*]const u8 = @ptrCast(texture.ptr);
    self.allocator.free(ptr[0 .. (texture.width * texture.height * 4)]);
}

pub fn textureFromTarget(_: *TestingBackend, texture: dvui.TextureTarget) !dvui.Texture {
    return .{ .ptr = texture.ptr, .width = texture.width, .height = texture.height };
}

pub fn renderTarget(_: *TestingBackend, _: ?dvui.TextureTarget) !void {}

pub fn clipboardText(self: *TestingBackend) std.mem.Allocator.Error![]const u8 {
    if (self.clipboard) |text| {
        return try self.arena.dupe(u8, text);
    }
    return "";
}

pub fn clipboardTextSet(self: *TestingBackend, text: []const u8) std.mem.Allocator.Error!void {
    if (self.clipboard) |prev_text| {
        self.allocator.free(prev_text);
    }
    self.clipboard = try self.allocator.dupe(u8, text);
}

pub fn openURL(_: *TestingBackend, _: []const u8) std.mem.Allocator.Error!void {}

pub fn preferredColorScheme(_: *TestingBackend) ?dvui.enums.ColorScheme {
    return null;
}

pub fn cursorShow(self: *TestingBackend, value: ?bool) bool {
    defer if (value) |val| {
        self.cursor_shown = val;
    };
    return self.cursor_shown;
}

pub fn refresh(_: *TestingBackend) void {}

pub fn backend(self: *TestingBackend) dvui.Backend {
    return dvui.Backend.init(self);
}
