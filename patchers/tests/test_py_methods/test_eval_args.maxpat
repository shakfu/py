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
			100.0,
			100.0,
			700.0,
			630.0
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
						20.0,
						10.0,
						640.0,
						40.0
					],
					"text": "eval / exec / cache arguments. Click top to bottom; compare the console with 'expect'.\nBefore the fix, every multi-atom or numeric message failed or used only its first atom.",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 2
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "newobj",
					"patching_rect": [
						20.0,
						500.0,
						110.0,
						22.0
					],
					"text": "py @name eval_args",
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
					"maxclass": "message",
					"patching_rect": [
						20.0,
						70.0,
						210.0,
						22.0
					],
					"text": "eval 1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-4",
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						70.0,
						400.0,
						20.0
					],
					"text": "eval: 1 (int atom)",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-5",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						100.0,
						210.0,
						22.0
					],
					"text": "eval -1",
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
						250.0,
						100.0,
						400.0,
						20.0
					],
					"text": "eval: -1",
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
						20.0,
						130.0,
						210.0,
						22.0
					],
					"text": "eval 1.5",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-8",
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						130.0,
						400.0,
						20.0
					],
					"text": "eval: 1.5 (float atom)",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-9",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						160.0,
						210.0,
						22.0
					],
					"text": "eval 1 + 2",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-10",
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						160.0,
						400.0,
						20.0
					],
					"text": "eval: 3, not 1 (three atoms)",
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
						20.0,
						190.0,
						210.0,
						22.0
					],
					"text": "exec x = 41",
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
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						190.0,
						400.0,
						20.0
					],
					"text": "done: bang (three atoms)",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-13",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						220.0,
						210.0,
						22.0
					],
					"text": "eval x + 1",
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
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						220.0,
						400.0,
						20.0
					],
					"text": "eval: 42, not 41",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-15",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						250.0,
						210.0,
						22.0
					],
					"text": "eval 0.1234567891 > 0.123457",
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
						250.0,
						250.0,
						400.0,
						20.0
					],
					"text": "eval: 1 (float digits kept)",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-17",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						280.0,
						210.0,
						22.0
					],
					"text": "eval \"1 + 2\"",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-18",
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						280.0,
						400.0,
						20.0
					],
					"text": "eval: 3 (one symbol with spaces)",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-19",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						310.0,
						210.0,
						22.0
					],
					"text": "eval \"len('a  b')\"",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-20",
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						310.0,
						400.0,
						20.0
					],
					"text": "eval: 4 (one symbol, two spaces kept)",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-21",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						340.0,
						210.0,
						22.0
					],
					"text": "exec 1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-22",
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						340.0,
						400.0,
						20.0
					],
					"text": "done: bang (a number is a valid statement)",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-23",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						370.0,
						210.0,
						22.0
					],
					"text": "eval \"sum([1, 2, 3])\"",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-24",
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						370.0,
						400.0,
						20.0
					],
					"text": "eval: 6 (Max splits at commas, so code with commas is quoted)",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-25",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						400.0,
						210.0,
						22.0
					],
					"text": "cache def f(x): return x + 1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-26",
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						400.0,
						400.0,
						20.0
					],
					"text": "done: bang (six atoms), no validation error",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-27",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						430.0,
						210.0,
						22.0
					],
					"text": "eval f(41)",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-28",
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						430.0,
						400.0,
						20.0
					],
					"text": "eval: 42 (f was cached and defined)",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-29",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						460.0,
						210.0,
						22.0
					],
					"text": "cache",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-30",
					"maxclass": "comment",
					"patching_rect": [
						250.0,
						460.0,
						400.0,
						20.0
					],
					"text": "error 'cache needs code to run', fail: bang",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-31",
					"maxclass": "newobj",
					"patching_rect": [
						20.0,
						540.0,
						70.0,
						22.0
					],
					"text": "print eval",
					"numinlets": 1,
					"numoutlets": 0
				}
			},
			{
				"box": {
					"id": "obj-32",
					"maxclass": "newobj",
					"patching_rect": [
						180.0,
						540.0,
						70.0,
						22.0
					],
					"text": "print fail",
					"numinlets": 1,
					"numoutlets": 0
				}
			},
			{
				"box": {
					"id": "obj-33",
					"maxclass": "newobj",
					"patching_rect": [
						300.0,
						540.0,
						70.0,
						22.0
					],
					"text": "print done",
					"numinlets": 1,
					"numoutlets": 0
				}
			},
			{
				"box": {
					"id": "obj-34",
					"maxclass": "comment",
					"patching_rect": [
						20.0,
						575.0,
						640.0,
						20.0
					],
					"text": "Only the last message ('cache' alone) should print 'fail: bang'.",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
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
						"obj-2",
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
						"obj-2",
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
						"obj-2",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-9",
						0
					],
					"destination": [
						"obj-2",
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
						"obj-2",
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
						"obj-2",
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
						"obj-2",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-17",
						0
					],
					"destination": [
						"obj-2",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-19",
						0
					],
					"destination": [
						"obj-2",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-21",
						0
					],
					"destination": [
						"obj-2",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-23",
						0
					],
					"destination": [
						"obj-2",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-25",
						0
					],
					"destination": [
						"obj-2",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-27",
						0
					],
					"destination": [
						"obj-2",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-29",
						0
					],
					"destination": [
						"obj-2",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-2",
						0
					],
					"destination": [
						"obj-31",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-2",
						1
					],
					"destination": [
						"obj-32",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-2",
						2
					],
					"destination": [
						"obj-33",
						0
					]
				}
			}
		]
	}
}
