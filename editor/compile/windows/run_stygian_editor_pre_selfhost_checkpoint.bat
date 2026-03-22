@echo off
setlocal
cd /d "%~dp0\..\..\.."

call editor\compile\windows\build_stygian_editor_release_gate.bat %*
if errorlevel 1 exit /b %ERRORLEVEL%

set BIN=editor\build\windows

call :run_exe stygian_editor_core_unit_suite
call :run_exe stygian_editor_project_roundtrip_smoke
call :run_exe stygian_editor_project_roundtrip_harness
call :run_exe stygian_editor_export_determinism_smoke
call :run_exe stygian_editor_export_c23_golden_smoke
call :run_exe stygian_editor_generated_scene_runtime_smoke
call :run_exe stygian_editor_project_hostile_load_smoke
call :run_exe stygian_editor_module_abi_compat_smoke
call :run_exe stygian_editor_performance_budget_smoke
call :run_exe stygian_editor_gradient_effect_parity_smoke
call :run_exe stygian_editor_mask_runtime_parity_smoke
call :run_exe stygian_editor_boolean_solver_parity_smoke
call :run_exe stygian_editor_text_shaping_overflow_parity_smoke
call :run_exe stygian_editor_variable_binding_parity_smoke
call :run_exe stygian_editor_component_detach_repair_parity_smoke
call :run_exe stygian_editor_advanced_control_recipes_smoke
call :run_exe stygian_editor_host_interop_contract_smoke
call :run_exe stygian_editor_tabs_behavior_smoke
call :run_exe stygian_editor_animation_transition_timeline_smoke
call :run_exe stygian_editor_phase75_variant_hardening_smoke
call :run_exe stygian_editor_phase6_components_styles_variables_smoke
call :run_exe stygian_editor_phase7_behavior_bridge_smoke
call :run_exe stygian_editor_self_host_layout_shell_smoke
call :run_exe stygian_editor_self_host_stage_migration_smoke
call :run_exe stygian_editor_self_host_escape_hatch_smoke
call :run_exe stygian_editor_self_host_daily_flow_smoke

echo [pre_selfhost_checkpoint] PASS
exit /b 0

:run_exe
echo [pre_selfhost_checkpoint] Running %1.exe
"%BIN%\%1.exe"
if errorlevel 1 (
  echo [pre_selfhost_checkpoint] FAIL %1.exe
  exit /b %ERRORLEVEL%
)
exit /b 0
