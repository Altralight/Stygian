const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const svg2tvg_dep = b.dependency("svg2tvg", .{
        .target = target,
        .optimize = optimize,
    });
    const dvui_root = "../../../build/_cmp_src/dvui";

    const build_options = b.addOptions();
    build_options.addOption(?bool, "zig_arena", null);
    build_options.addOption(?[]const u8, "snapshot_image_suffix", null);
    build_options.addOption(?[]const u8, "image_dir", null);
    build_options.addOption(?u8, "log_stack_trace", null);
    build_options.addOption(?bool, "log_error_trace", null);

    const backend_mod = b.createModule(.{
        .root_source_file = b.path("src/backend.zig"),
        .target = target,
        .optimize = optimize,
    });

    const dvui_mod = b.createModule(.{
        .root_source_file = b.path(dvui_root ++ "/src/dvui.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    dvui_mod.addImport("backend", backend_mod);
    dvui_mod.addImport("svg2tvg", svg2tvg_dep.module("svg2tvg"));
    dvui_mod.addOptions("build_options", build_options);
    dvui_mod.addIncludePath(b.path(dvui_root ++ "/src/stb"));
    dvui_mod.addIncludePath(b.path(dvui_root ++ "/src/tfd"));
    dvui_mod.addCSourceFiles(.{
        .root = b.path(dvui_root ++ "/src/stb"),
        .files = &.{
            "stb_image_impl.c",
            "stb_image_write_impl.c",
            "stb_truetype_impl.c",
        },
        .flags = &.{},
    });
    dvui_mod.addCSourceFiles(.{
        .root = b.path(dvui_root ++ "/src/tfd"),
        .files = &.{"tinyfiledialogs.c"},
        .flags = &.{},
    });
    if (target.result.os.tag == .windows) {
        dvui_mod.linkSystemLibrary("comdlg32", .{});
        dvui_mod.linkSystemLibrary("ole32", .{});
    }

    backend_mod.addImport("dvui", dvui_mod);

    const exe = b.addExecutable(.{
        .name = "dvui_headless_bench",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });
    exe.root_module.addImport("dvui", dvui_mod);
    exe.root_module.addImport("backend", backend_mod);
    exe.linkLibC();

    b.installArtifact(exe);
}
