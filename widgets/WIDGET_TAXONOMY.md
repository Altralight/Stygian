# Stygian Widget Taxonomy - 200+ Components

## Shipped (v0.1.0 baseline)

- [Finished] Button (`stygian_button`, `stygian_button_ex`)
- [Finished] Slider (`stygian_slider`, `stygian_slider_ex`)
- [Finished] Checkbox and radio
- [Finished] Text input and multiline text area
- [Finished] Vertical scrollbar (`stygian_scrollbar_v`)
- [Finished] Tooltip (`stygian_tooltip`)
- [Finished] Context menu (`stygian_context_menu_*`)
- [Finished] Modal (`stygian_modal_*`)
- [Finished] Panel begin/end
- [Finished] Perf widget (`stygian_perf_widget`)
- [Finished] File explorer and breadcrumb
- [Finished] Output and problems panels
- [Finished] Debug toolbar and call stack
- [Finished] Coordinate input and snap settings
- [Finished] CAD gizmo controls and layer manager
- [Finished] Scene viewport, scene hierarchy, inspector, asset browser, console log
- [Finished] Split panel, menu bar, toolbar
- [In Progress] Node graph editor path (helpers, iteration, node/wire rendering, culling)

## Vision Statement

Stygian should present a serious shipped baseline and a credible roadmap, not a flat wall of unchecked fantasy. The point of this taxonomy is to separate what exists today from what is partially underway and what is parked for later.

## Status Legend

- `[Finished]` means public API and implementation already exist in the current baseline.
- `[In Progress]` means there is meaningful code, scaffolding, or adjacent shipped behavior, but the named widget is not fully delivered as a first-class finished component.
- `[Later]` means roadmap only.

## Research Summary

| Domain | Source | Component Count |
|--------|--------|----------------|
| **ImGui** | Immediate-mode GUI | ~54 core widgets |
| **GitHub Primer** | Design system | 50+ components |
| **shadcn/ui** | React/Tailwind | 59 official |
| **Ant Design** | Enterprise UI | 50+ components |
| **D3.js** | Data visualization | 100+ chart types |
| **Plotly** | Scientific charts | 40+ chart types |
| **VSCode** | Editor UI | 15+ contribution points |
| **DAW/Audio** | Signal processing | 20+ specialized widgets |
| **Node Editors** | React Flow, Rete.js | 10+ node-specific widgets |
| **Social Media** | Feeds, chat, timeline | 15+ specialized widgets |

**Stygian Target: 200+ widgets across 15 categories**

---

## Category 1: Core Input & Interaction (60 widgets)

### Buttons (12)
- [Finished] Button (primary, secondary, tertiary)
- [Later] SmallButton, LargeButton
- [Later] IconButton, ImageButton
- [Later] ArrowButton (up, down, left, right)
- [Later] InvisibleButton (hit testing)
- [Later] ToggleButton, SplitButton
- [Later] FloatButton (FAB)
- [Later] ButtonGroup (horizontal, vertical)

### Checkboxes & Radio (6)
- [Finished] Checkbox, CheckboxFlags
- [Later] CheckboxGroup
- [Finished] RadioButton, RadioGroup
- [Later] Switch / Toggle

### Sliders & Ranges (20)
- [In Progress] SliderInt, SliderInt2, SliderInt3, SliderInt4
- [Finished] SliderFloat, SliderFloat2, SliderFloat3, SliderFloat4
- [Later] SliderAngle
- [Later] VSliderInt, VSliderFloat (vertical)
- [Later] RangeSlider (dual-handle)
- [Later] CircularSlider (radial)
- [Later] Knob (rotary, DAW-style)

### Drags & Spinners (14)
- [Later] DragInt, DragInt2, DragInt3, DragInt4
- [Later] DragFloat, DragFloat2, DragFloat3, DragFloat4
- [Later] DragIntRange2, DragFloatRange2
- [Later] DragAngle
- [Later] Spinner (increment/decrement)

### Rating & Segmented (8)
- [Later] Rating (stars)
- [Later] SegmentedControl
- [Later] Stepper (iOS-style)
- [Later] PinInput (OTP/code)
- [Later] Dial (rotary value)
- [Later] Joystick (2D input)
- [Later] Touchpad (gesture area)
- [Later] GestureZone

---

## Category 2: Text & Data Entry (25 widgets)

### Text Inputs (15)
- [Finished] InputText, InputTextMultiline
- [Later] InputTextWithHint
- [Later] PasswordInput (masked)
- [Later] SearchInput (with icon)
- [Later] NumberInput
- [Later] InputInt, InputInt2, InputInt3, InputInt4
- [Later] InputFloat, InputFloat2, InputFloat3, InputFloat4
- [Later] InputDouble
- [Later] InputScalar, InputScalarN

### Advanced Text (10)
- [Later] AutoComplete
- [Later] TagInput (chip input)
- [Later] Mentions (@-mentions)
- [In Progress] CodeEditor (syntax highlighting)
- [Later] MarkdownEditor
- [Later] RichTextEditor (WYSIWYG)
- [Finished] TextArea (resizable)
- [Later] FormattedInput (masks, patterns)
- [Later] CurrencyInput
- [Later] PhoneInput

---

## Category 3: Selectors & Pickers (18 widgets)

### Dropdowns & Lists (8)
- [Later] Combo, ListBox
- [Later] MultiSelect
- [Later] Selectable
- [Later] BeginListBox/EndListBox
- [Later] Cascader (hierarchical select)
- [Later] Transfer (dual-list)
- [Later] SelectPanel

### Pickers (10)
- [Later] DatePicker, TimePicker, DateTimePicker
- [Later] DateRangePicker, TimeRangePicker
- [Later] ColorPicker3, ColorPicker4
- [Later] ColorEdit3, ColorEdit4
- [Later] ColorButton
- [Later] FilePicker / FileUpload
- [Later] ImagePicker
- [Later] FontPicker
- [Later] IconPicker
- [Later] EmojiPicker

---

## Category 4: Data Display & Tables (20 widgets)

### Tables & Grids (8)
- [Later] BeginTable/EndTable (full table API)
- [Later] DataGrid (advanced, sortable, filterable)
- [Later] VirtualTable (large datasets)
- [Later] EditableTable (inline editing)
- [Later] PivotTable
- [Later] Spreadsheet (Excel-like)
- [Later] DataTable (GitHub Primer style)
- [Later] TreeTable (hierarchical data)

### Lists & Collections (12)
- [Later] List (simple, virtualized)
- [Later] VirtualList (infinite scroll)
- [Later] Timeline
- [Later] Feed (social media style)
- [Later] Card, CardGrid
- [Later] Masonry (Pinterest layout)
- [Later] Gallery, ImageGallery
- [Later] Carousel / Slider
- [Later] Kanban (drag-drop board)
- [In Progress] Calendar (month, week, day views)
- [Later] Agenda (event list)
- [Later] ActivityLog

---

## Category 5: Trees & Hierarchies (8 widgets)

- [Later] TreeNode, TreePop
- [Later] TreeView (full tree API)
- [Later] CollapsingHeader
- [Later] SetNextItemOpen
- [In Progress] FileTree (VSCode-style)
- [In Progress] FolderTree
- [Later] Accordion / Collapse
- [Later] NestedList

---

## Category 6: Navigation & Menus (18 widgets)

### Menus (8)
- [In Progress] BeginMenu/EndMenu, MenuItem
- [Finished] BeginMenuBar/EndMenuBar
- [Finished] ContextMenu (right-click)
- [Later] DropdownMenu
- [Later] ActionMenu (GitHub style)
- [Later] CommandPalette (Cmd+K)
- [Later] QuickPick (VSCode style)
- [Later] Spotlight (macOS style)

### Navigation (10)
- [In Progress] BeginTabBar/EndTabBar
- [In Progress] BeginTabItem/EndTabItem
- [In Progress] Tabs (horizontal, vertical)
- [Finished] Breadcrumb
- [Later] Pagination
- [Later] Steps / Stepper
- [Later] Anchor (table of contents)
- [Later] NavList (GitHub style)
- [Later] Sidebar, BottomNavigation
- [Later] UnderlineNav, UnderlinePanels

---

## Category 7: Overlays & Feedback (20 widgets)

### Modals & Dialogs (8)
- [Finished] Modal / Dialog
- [Later] ConfirmDialog
- [Later] Popconfirm
- [Later] Drawer (side panel)
- [Later] Sheet (bottom sheet)
- [Later] Popover
- [Finished] Tooltip
- [Later] AnchoredOverlay

### Notifications (12)
- [Later] Alert / Banner
- [Later] Toast / Notification
- [Later] Snackbar
- [Later] InlineMessage
- [Later] Message (floating)
- [Later] Progress (linear)
- [Later] ProgressCircle (circular)
- [Later] ProgressBar
- [Later] Spinner / Loader
- [Later] Skeleton (loading placeholder)
- [Later] Backdrop
- [Later] Watermark

---

## Category 8: Layout & Containers (15 widgets)

- [Finished] Panel / Container
- [Later] Box, Stack (VStack, HStack)
- [In Progress] Grid, Flex
- [Finished] Splitter (resizable divider)
- [Later] SplitPageLayout
- [Later] PageLayout, PageHeader
- [In Progress] ScrollArea, ScrollView
- [Later] Divider / Separator
- [Later] Spacer, Dummy
- [Later] Blankslate (empty state)
- [Later] Affix (sticky)
- [Later] BackTop (scroll to top)
- [Later] Resizable (resize handle)
- [In Progress] DockSpace (docking layout)
- [In Progress] Workspace (multi-panel)

---

## Category 9: Charts & Data Visualization (40 widgets)

### Basic Charts (10)
- [Later] LineChart, AreaChart
- [Later] BarChart (vertical, horizontal)
- [Later] StackedBarChart
- [Later] GroupedBarChart
- [Later] PieChart, DonutChart
- [Later] ScatterPlot, BubbleChart
- [Later] Histogram

### Statistical Charts (10)
- [Later] BoxPlot, ViolinPlot
- [Later] Heatmap, Correlogram
- [Later] DensityPlot (1D, 2D)
- [Later] ContourPlot
- [Later] Candlestick (financial)
- [Later] Waterfall
- [Later] FunnelChart
- [Later] RadarChart, PolarChart
- [Later] ParallelCoordinates

### Advanced Visualizations (10)
- [Later] Treemap, Sunburst
- [Later] CirclePacking
- [Later] SankeyDiagram
- [Later] ChordDiagram
- [Later] ForceDirectedGraph
- [Later] NetworkGraph
- [Later] Dendrogram
- [Later] Icicle
- [Later] Streamgraph
- [Later] RidgePlot

### Time Series & Geo (10)
- [Later] TimeSeriesChart
- [Later] Sparkline
- [Later] Gauge, Indicator (KPI)
- [Later] ChoroplethMap
- [Later] BubbleMap, HexbinMap
- [Later] HeatMap (geographic)
- [Later] FlowMap
- [Later] VectorField
- [Later] Wordcloud
- [Later] PlotLines, PlotHistogram (ImGui style)

---

## Category 10: Node Editors & Graphs (12 widgets)

- [In Progress] NodeGraph (full editor)
- [In Progress] Node (draggable)
- [Later] NodeSocket (input/output)
- [In Progress] NodeConnection (edge/wire)
- [Finished] NodeCanvas (background grid)
- [Later] Minimap (node overview)
- [Later] NodePalette (node library)
- [Later] NodeInspector (properties)
- [Later] FlowChart
- [Later] StateMachine (visual)
- [Later] BehaviorTree
- [Later] Blueprint (UE4-style)

---

## Category 11: Audio & Signal Processing (15 widgets)

### DAW Controls (8)
- [Later] Knob (rotary, skeuomorphic)
- [Later] Fader (vertical, horizontal)
- [Later] VUMeter, PeakMeter
- [Later] LevelMeter (multi-channel)
- [Later] TransportControls (play, stop, record)
- [Later] WaveformDisplay
- [Later] SpectrogramDisplay
- [Later] SpectrumAnalyzer

### Audio Editing (7)
- [Later] AudioTimeline
- [Later] MixerChannel
- [Later] EQCurve (graphical EQ)
- [Later] CompressorCurve
- [Later] EnvelopeEditor (ADSR)
- [Later] PianoRoll (MIDI)
- [Later] SampleEditor

---

## Category 12: Social Media & Collaboration (15 widgets)

### Feeds & Timelines (5)
- [Later] Feed (infinite scroll)
- [Later] Timeline (chronological)
- [Later] ActivityFeed
- [Later] NewsFeed
- [Later] MasonryFeed

### Chat & Messaging (10)
- [Later] ChatBubble, MessageBubble
- [Later] ChatInput
- [Later] ChatTimeline
- [Later] TypingIndicator
- [Later] ReadReceipt
- [Later] ReactionPicker (emoji)
- [Later] ThreadView
- [Later] ChannelList
- [Later] UserList
- [Later] PresenceIndicator

---

## Category 13: Sports & Analytics (12 widgets)

### Football/Soccer Specific (8)
- [Later] PitchHeatmap (player positions)
- [Later] PassNetwork
- [Later] ShotMap
- [Later] PlayerTrails (movement)
- [Later] FormationDiagram
- [Later] MatchTimeline
- [Later] LeagueTable
- [Later] PlayerStatsCard

### General Sports (4)
- [Later] ScoreWidget
- [Later] LiveScoreboard
- [Later] StandingsTable
- [Later] H2HComparison (head-to-head)

---

## Category 14: Editor & IDE Components (15 widgets)

### VSCode-style (10)
- [In Progress] CodeEditor (full featured)
- [Later] SyntaxHighlighter
- [Later] LineNumbers
- [Later] Minimap (code overview)
- [Finished] Breadcrumb (file path)
- [Finished] FileExplorer (tree)
- [Later] SearchPanel
- [Later] ReplacePanel
- [Later] TerminalEmulator
- [In Progress] DebugPanel

### Code Features (5)
- [Later] CodeLens (inline info)
- [Later] InlayHints
- [Later] Gutter (line decorations)
- [Later] DiffViewer (side-by-side)
- [Later] BlameView (git annotations)

---

## Category 15: Specialized & Advanced (17 widgets)

### Media (5)
- [Later] VideoPlayer
- [Later] AudioPlayer
- [Later] ImageViewer (zoom, pan)
- [Later] PDFViewer
- [Later] 3DViewer (model preview)

### Utilities (12)
- [Later] QRCode, Barcode
- [Later] Tour (onboarding)
- [Later] Spotlight (feature highlight)
- [Later] Hotkey (keyboard shortcut display)
- [Later] Badge, Tag, Chip
- [Later] Avatar, AvatarStack
- [Later] StateLabel (GitHub style)
- [Later] CounterLabel
- [Later] CircleBadge
- [Later] Token (removable tag)
- [Later] Truncate (text overflow)
- [Later] RelativeTime (time ago)

---

## Total Widget Count: **207 widgets**

---

## Implementation Phases

### Phase 1: Shipped Baseline Hardening

- finish the current shipped set to production quality
- keep demos, tests, and docs aligned with what really exists

### Phase 2: Adjacent Expansions

- tabs, dockspace, scroll areas, code-editor-adjacent tooling, and full node graph behavior

### Phase 3: Breadth Growth

- fill in missing data display, navigation, and editor categories

### Phase 4: Specialized Domains

- charts, audio, social, sports, and media-specific widgets

---

## Design Principles

### 1. Keep the shipped list honest

If it is not public and implemented, it does not get marked finished.

### 2. Expand around real tool workflows first

Stygian is strongest today in native-tooling UI, not generic component-box checking.

### 3. Preserve DDI and retained-runtime behavior

New widgets should fit the invalidation-driven model instead of smuggling in redraw-on-input behavior.

---

**Stygian can still aim big without lying about what is already there.**
