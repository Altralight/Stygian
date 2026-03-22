# Editor Project Schema v0

## Purpose

This sidecar exists for editor persistence only.

- Runtime output remains pure generated Stygian C23.
- The sidecar keeps authoring intent so the editor can reload state.

## Schema Header

- `schema_name`: `"stygian-editor-project"`
- `schema_major`: `0`
- `schema_minor`: `0`

Loader behavior:

- major mismatch: fail
- minor newer than loader: warn and attempt load

## Top-Level Fields

- `viewport`: string record
- `grid`: string record
- `ruler`: string record
- `snap_sources`: string record
- `next_ids`: string record
- `selection`: string record
- `color_tokens`: array of string records
- `guides`: array of string records
- `nodes`: array of string records
- `behaviors`: array of string records

## Record Encoding

Each record is a semicolon-delimited key/value list:

`key=value;key=value;...`

Reserved separators are percent-escaped as `%HH` (hex byte):

- `%`
- `;`
- `=`
- `,`
- `|`
- `"`
- `\`
- control characters

## Record Types

### `viewport`

`w=<f>;h=<f>;px=<f>;py=<f>;z=<f>`

### `grid`

`en=<0|1>;sub=<0|1>;maj=<f>;div=<u32>;min=<f>;tol=<f>`

### `ruler`

`unit=<u32>`

### `snap_sources`

`grid=<0|1>;guides=<0|1>;bounds=<0|1>;parent=<0|1>`

### `next_ids`

`node=<u32>;path=<u32>;behavior=<u32>;guide=<u32>`

### `selection`

`primary=<u32>;ids=<u32|u32|...>`

### `color_tokens[]`

`n=<escaped_name>;c=<r,g,b,a>`

### `guides[]`

`id=<u32>;axis=<0|1>;pos=<f>;pid=<u32>`

### `nodes[]`

Common fields for all node kinds:

`id=<u32>;pid=<u32>;k=<rect|ellipse|path|frame|text|image|line|arrow|polygon|star|arc|group|component_def|component_instance>;v=<0|1>;lock=<0|1>;z=<f>;val=<f>;rot=<f>;name=<escaped_name>;hc=<u32>;vc=<u32>;cl=<f>;cr=<f>;ct=<f>;cb=<f>;ccx=<f>;ccy=<f>;cxr=<f>;cyr=<f>;cwr=<f>;chr=<f>;la=<0|1>;lsw=<u32>;lsh=<u32>;mnw=<f>;mxw=<f>;mnh=<f>;mxh=<f>;tok=<escaped_token>`

Rect fields:

`x=<f>;y=<f>;w=<f>;h=<f>;rad=<tl,tr,br,bl>;fill=<r,g,b,a>`

Ellipse fields:

`x=<f>;y=<f>;w=<f>;h=<f>;fill=<r,g,b,a>`

Path fields:

`closed=<0|1>;th=<f>;stroke=<r,g,b,a>;pts=<x,y|x,y|...>`

Path constraints:

- at least 2 points

Frame fields:

`x=<f>;y=<f>;w=<f>;h=<f>;clip=<0|1>;alm=<u32>;apl=<f>;apr=<f>;apt=<f>;apb=<f>;ag=<f>;apa=<u32>;aca=<u32>;ofp=<u32>;sx=<f>;sy=<f>;fill=<r,g,b,a>`

Text fields:

`x=<f>;y=<f>;w=<f>;h=<f>;size=<f>;fill=<r,g,b,a>;txt=<escaped_text>`

Image fields:

`x=<f>;y=<f>;w=<f>;h=<f>;fit=<u32>;src=<escaped_path>`

Line fields:

`x1=<f>;y1=<f>;x2=<f>;y2=<f>;th=<f>;stroke=<r,g,b,a>`

Arrow fields:

`x1=<f>;y1=<f>;x2=<f>;y2=<f>;th=<f>;hs=<f>;stroke=<r,g,b,a>`

Polygon fields:

`x=<f>;y=<f>;w=<f>;h=<f>;sides=<u32>;corner=<f>;fill=<r,g,b,a>`

Star fields:

`x=<f>;y=<f>;w=<f>;h=<f>;pts=<u32>;inner=<f>;fill=<r,g,b,a>`

Arc fields:

`x=<f>;y=<f>;w=<f>;h=<f>;start=<f>;sweep=<f>;th=<f>;stroke=<r,g,b,a>`

Group fields:

`x=<f>;y=<f>;w=<f>;h=<f>;clip=<0|1>`

Component definition fields:

`x=<f>;y=<f>;w=<f>;h=<f>;sym=<escaped_symbol>`

Component instance fields:

`x=<f>;y=<f>;w=<f>;h=<f>;ref=<escaped_symbol>`

### `behaviors[]`

Current action kind is animate-only in v0:

`id=<u32>;trn=<trigger_node_id>;ev=<event>;tgt=<target_node_id>;prop=<property>;from=<f|null>;to=<f>;dur=<u32>;ease=<easing>`

Supported events:

- `press`
- `release`
- `hover_enter`
- `hover_leave`
- `drag_start`
- `drag_move`
- `drag_end`
- `scroll`
- `value_changed`

Supported properties:

- `x`
- `y`
- `width`
- `height`
- `opacity`
- `radius_tl`
- `radius_tr`
- `radius_br`
- `radius_bl`
- `value`

Supported easing:

- `linear`
- `out_cubic`
- `in_out_cubic`

## Validation Rules

- Node IDs must be non-zero and unique.
- Behavior IDs must be non-zero.
- Behavior trigger and target nodes must exist.
- Guide IDs must be non-zero and unique.
- If a guide has `pid != 0`, that parent node must exist.
- Color token names are capped by editor runtime buffer.
- Path point count must fit editor point capacity.

## Identity, Symbols, And Regeneration Rules (T101)

### Node identity

- Node IDs are monotonic and non-reused inside a project lifecycle.
- Deleting a node does not free its ID for reuse.
- Reload keeps persisted IDs exactly as stored.
- `next_ids.node` must always be greater than any existing node ID.

### Generated symbol naming

- Runtime identity in generated C23 is `node_id`-based, not label-based.
- Generated dispatch helpers map by node ID (`stygian_editor_generated_node_index`).
- Any future display label support must be treated as presentation metadata only:
  label changes must not change node IDs or generated identity keys.

### Regeneration determinism

- For an unchanged editor model, generated C23 output must be byte-identical.
- Save-load without content edits must preserve generated C23 output exactly.
- Node emission order is scene-order driven (`nodes[]` record order), so stable
  persisted order yields stable generated code order.

## API Surface

Implemented by:

- `stygian_editor_build_project_json(...)`
- `stygian_editor_load_project_json(...)`
- `stygian_editor_save_project_file(...)`
- `stygian_editor_load_project_file(...)`

Headers:

- `D:\Projects\Code\Stygian\editor\include\stygian_editor.h`
- `D:\Projects\Code\Stygian\editor\include\stygian_editor_module.h`
