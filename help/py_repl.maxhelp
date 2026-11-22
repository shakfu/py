{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 2,
			"revision": 1,
			"architecture": "x64",
			"modernui": 1
		},
		"classnamespace": "box",
		"rect": [
			100.0,
			100.0,
			940.0,
			700.0
		],
		"autosave": 0,
		"boxes": [
			{
				"box": {
					"id": "obj-1",
					"maxclass": "comment",
					"patching_rect": [
						20,
						8,
						420,
						18.0
					],
					"text": "py_repl: a Python console for [py]",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1,
					"fontface": 1,
					"fontsize": 16.0
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "comment",
					"patching_rect": [
						20,
						34,
						460,
						18.0
					],
					"text": "Runs code in the namespace of the py object named by @py.",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-3",
					"maxclass": "attrui",
					"patching_rect": [
						20,
						62,
						460,
						22.0
					],
					"attr": "py",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-4",
					"maxclass": "attrui",
					"patching_rect": [
						20,
						88,
						460,
						22.0
					],
					"attr": "theme",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-5",
					"maxclass": "attrui",
					"patching_rect": [
						20,
						114,
						460,
						22.0
					],
					"attr": "promptcolor",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-6",
					"maxclass": "py_repl",
					"patching_rect": [
						20,
						145,
						460,
						300
					],
					"numinlets": 1,
					"numoutlets": 0,
					"py": "repl_help"
				}
			},
			{
				"box": {
					"id": "obj-7",
					"maxclass": "newobj",
					"patching_rect": [
						20,
						460,
						130,
						22.0
					],
					"text": "py @name repl_help",
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
					"id": "obj-8",
					"maxclass": "comment",
					"patching_rect": [
						160,
						460,
						320,
						18.0
					],
					"text": "the namespace the console runs in",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-9",
					"maxclass": "newobj",
					"patching_rect": [
						20,
						490,
						100,
						22.0
					],
					"text": "print repl_help",
					"numinlets": 1,
					"numoutlets": 0
				}
			},
			{
				"box": {
					"id": "obj-10",
					"maxclass": "comment",
					"patching_rect": [
						125,
						490,
						360,
						18.0
					],
					"text": "api output from code run in the console",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-11",
					"maxclass": "message",
					"patching_rect": [
						20,
						530,
						130,
						22.0
					],
					"text": "run import math",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-12",
					"maxclass": "message",
					"patching_rect": [
						20,
						558,
						130,
						22.0
					],
					"text": "run math.pi * 2",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-13",
					"maxclass": "message",
					"patching_rect": [
						20,
						586,
						130,
						22.0
					],
					"text": "run \"max(1, 2)\"",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-14",
					"maxclass": "message",
					"patching_rect": [
						20,
						614,
						130,
						22.0
					],
					"text": "clear",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-15",
					"maxclass": "message",
					"patching_rect": [
						20,
						642,
						130,
						22.0
					],
					"text": "history clear",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-16",
					"maxclass": "comment",
					"patching_rect": [
						160,
						530,
						320,
						18.0
					],
					"text": "run: as if typed and submitted",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-17",
					"maxclass": "comment",
					"patching_rect": [
						160,
						586,
						320,
						18.0
					],
					"text": "quote code with commas: Max splits a message at them",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-18",
					"maxclass": "comment",
					"patching_rect": [
						160,
						614,
						320,
						18.0
					],
					"text": "clear the transcript",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-19",
					"maxclass": "comment",
					"patching_rect": [
						160,
						642,
						320,
						18.0
					],
					"text": "forget the saved history",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-20",
					"maxclass": "comment",
					"patching_rect": [
						510,
						62,
						400,
						18.0
					],
					"text": "Keys",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1,
					"fontface": 1
				}
			},
			{
				"box": {
					"id": "obj-21",
					"maxclass": "comment",
					"patching_rect": [
						510,
						84,
						400,
						144.0
					],
					"text": "Return          submit complete code, else a new line\nShift+Return    new line\nCmd+Return      submit\nUp, Down        history (on the first or last input row)\nTab             complete names from the namespace\nCtrl+C          stop a run; idle: discard the input\nCmd+K, Ctrl+L   clear the transcript\nPageUp, Down    scroll",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 8,
					"fontname": "Menlo",
					"fontsize": 11.0
				}
			},
			{
				"box": {
					"id": "obj-22",
					"maxclass": "comment",
					"patching_rect": [
						510,
						250,
						400,
						18.0
					],
					"text": "Notes",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1,
					"fontface": 1
				}
			},
			{
				"box": {
					"id": "obj-23",
					"maxclass": "comment",
					"patching_rect": [
						510,
						272,
						400,
						324.0
					],
					"text": "Code runs on Max's main thread, so api calls are safe,\nbut the patch UI (drawing, mouse, keys) waits until the\nrun ends; output appears then. Audio and, with Overdrive\non, scheduled events keep running. Keys typed during a\nrun are kept.\n\nCtrl+C stops a run between bytecodes: time.sleep and\nlong C calls (numpy, I/O) finish first. It only acts\nwhile Max is the frontmost app.\n\nA block (def, for, if) ends with a blank line.\n\nHistory and the unsent input are saved with the patch;\nthe transcript is not.\n\nThe title shows 'not found' when no py object has the\n@py name, and 'abi mismatch' when py is older than\npy_repl.",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 18
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": [
						"obj-3",
						0
					],
					"destination": [
						"obj-6",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-4",
						0
					],
					"destination": [
						"obj-6",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-5",
						0
					],
					"destination": [
						"obj-6",
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
						"obj-9",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-11",
						0
					],
					"destination": [
						"obj-6",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-12",
						0
					],
					"destination": [
						"obj-6",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-13",
						0
					],
					"destination": [
						"obj-6",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-14",
						0
					],
					"destination": [
						"obj-6",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-15",
						0
					],
					"destination": [
						"obj-6",
						0
					]
				}
			}
		]
	}
}
