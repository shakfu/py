{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 8,
			"minor": 5,
			"revision": 3,
			"architecture": "x64",
			"modernui": 1
		},
		"classnamespace": "box",
		"rect": [
			80.0,
			80.0,
			1150.0,
			560.0
		],
		"default_fontsize": 12.0,
		"default_fontface": 0,
		"default_fontname": "Arial",
		"gridsize": [
			15.0,
			15.0
		],
		"boxes": [
			{
				"box": {
					"id": "obj-1",
					"maxclass": "comment",
					"patching_rect": [
						20,
						10,
						900,
						18.0
					],
					"text": "py_repl test. Lock the patch, click into the console, then work down the list.",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "newobj",
					"patching_rect": [
						20,
						40,
						140,
						22.0
					],
					"text": "py @name repl_demo",
					"numinlets": 1,
					"numoutlets": 3,
					"outlettype": [
						"",
						"bang",
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "obj-3",
					"maxclass": "py_repl",
					"patching_rect": [
						20,
						75,
						520,
						380
					],
					"numinlets": 1,
					"numoutlets": 0,
					"py": "repl_demo"
				}
			},
			{
				"box": {
					"id": "obj-4",
					"maxclass": "comment",
					"patching_rect": [
						560,
						75,
						560,
						306.0
					],
					"text": "1  1 + 2  Return                              -> 3\n2  def f(x):  Return  return x * 2  Return  Return    (auto indent; a blank line ends the block)\n   f(21)                                         -> 42\n3  print('out'); import sys; print('err', file=sys.stderr)   -> out, then err in red\n4  1 / 0                                         -> traceback in red, starting at File \"<py_repl>\"\n5  Up / Down                                     -> history, title shows 'history n/m'; Down past the newest restores the draft\n6  import math  then  math.fl  Tab               -> popup with floor; Return inserts math.floor\n7  while True: pass   then Ctrl+C                -> KeyboardInterrupt within a moment, no stray line after it\n8  type something, Ctrl+C                        -> the input is echoed, KeyboardInterrupt, input cleared\n9  for i in range(3000): print(i)                -> view follows the output to 2999\n10 scroll up with the wheel, click a transcript line, Cmd+C, click input, Cmd+V -> pastes the copied text\n11 Cmd+K                                         -> transcript cleared\n12 click the run message below                   -> the result appears as if typed\n13 set the py_repl's @py to nope in the inspector -> title 'py: nope | not found'; Return keeps the input\n14 save, close, reopen this patch                -> history (Up) and the unsent input are restored\n15 start step 7, Cmd+Tab to another app, press Ctrl+C there, come back, press it in Max -> only the press in Max interrupts\n16 set @promptcolor in the inspector               -> >>> and ... change color; set @theme -> they follow the theme's comment color",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 17
				}
			},
			{
				"box": {
					"id": "obj-5",
					"maxclass": "message",
					"patching_rect": [
						20,
						470,
						160,
						22.0
					],
					"text": "run sum(range(10))",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-6",
					"maxclass": "comment",
					"patching_rect": [
						185,
						470,
						200,
						18.0
					],
					"text": "12: expect 45",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-7",
					"maxclass": "message",
					"patching_rect": [
						20,
						500,
						50,
						22.0
					],
					"text": "clear",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": [
						"obj-5",
						0
					],
					"destination": [
						"obj-3",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-7",
						0
					],
					"destination": [
						"obj-3",
						0
					]
				}
			}
		]
	}
}
