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
			960.0,
			1060.0
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
						900.0,
						40.0
					],
					"text": "GIL tests. Run test 1 first: test 2 can leave the scheduler thread stuck.\nEach expectation names the result if py releases the GIL (OK) or holds it (BUG).",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 2
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "comment",
					"patching_rect": [
						20.0,
						60.0,
						400.0,
						100.0
					],
					"text": "1. Python thread (Overdrive either way)\na load, b start, wait 5 s without clicking, c rate\nOK: rate near 100 (sleep overshoot lowers it)\nBUG: rate near 0, rising only while you click\nd stop the thread",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 5
				}
			},
			{
				"box": {
					"id": "obj-3",
					"maxclass": "newobj",
					"patching_rect": [
						20.0,
						320.0,
						130.0,
						22.0
					],
					"text": "py @name gil_thread",
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
					"id": "obj-4",
					"maxclass": "newobj",
					"patching_rect": [
						20.0,
						360.0,
						110.0,
						22.0
					],
					"text": "print gil_thread",
					"numinlets": 1,
					"numoutlets": 0,
					"outlettype": []
				}
			},
			{
				"box": {
					"id": "obj-5",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						170.0,
						130.0,
						22.0
					],
					"text": "load test_gil.py",
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
					"maxclass": "message",
					"patching_rect": [
						20.0,
						205.0,
						130.0,
						22.0
					],
					"text": "eval start()",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-7",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						240.0,
						130.0,
						22.0
					],
					"text": "eval rate()",
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
					"maxclass": "message",
					"patching_rect": [
						20.0,
						275.0,
						130.0,
						22.0
					],
					"text": "eval stop()",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-9",
					"maxclass": "comment",
					"patching_rect": [
						160.0,
						170.0,
						20.0,
						20.0
					],
					"text": "a",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-10",
					"maxclass": "comment",
					"patching_rect": [
						160.0,
						205.0,
						20.0,
						20.0
					],
					"text": "b",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-11",
					"maxclass": "comment",
					"patching_rect": [
						160.0,
						240.0,
						20.0,
						20.0
					],
					"text": "c",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-12",
					"maxclass": "comment",
					"patching_rect": [
						160.0,
						275.0,
						20.0,
						20.0
					],
					"text": "d",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-13",
					"maxclass": "comment",
					"patching_rect": [
						460.0,
						60.0,
						460.0,
						140.0
					],
					"text": "2. Scheduler thread (needs Overdrive on, audio off)\na Overdrive on, b toggle on: sent counts ticks, done counts results,\nheartbeat counts a metro that never touches py\nOK: all three keep rising\nBUG: all three stop: the scheduler thread waits for the GIL\nRecover: toggle off, click c (kick) until done catches up, Overdrive off.\nIf Max still hangs, force quit.",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 7
				}
			},
			{
				"box": {
					"id": "obj-14",
					"maxclass": "message",
					"patching_rect": [
						460.0,
						210.0,
						120.0,
						22.0
					],
					"text": "; max overdrive 1",
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
						600.0,
						210.0,
						120.0,
						22.0
					],
					"text": "; max overdrive 0",
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
						460.0,
						235.0,
						20.0,
						20.0
					],
					"text": "a",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-17",
					"maxclass": "toggle",
					"patching_rect": [
						460.0,
						265.0,
						24.0,
						24.0
					],
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"int"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-18",
					"maxclass": "comment",
					"patching_rect": [
						490.0,
						265.0,
						20.0,
						20.0
					],
					"text": "b",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-19",
					"maxclass": "newobj",
					"patching_rect": [
						460.0,
						300.0,
						60.0,
						22.0
					],
					"text": "metro 10",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "obj-20",
					"maxclass": "newobj",
					"patching_rect": [
						460.0,
						330.0,
						45.0,
						22.0
					],
					"text": "t b b",
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"bang",
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "obj-21",
					"maxclass": "message",
					"patching_rect": [
						460.0,
						360.0,
						70.0,
						22.0
					],
					"text": "eval 1+1",
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
					"maxclass": "newobj",
					"patching_rect": [
						460.0,
						420.0,
						130.0,
						22.0
					],
					"text": "py @name gil_sched",
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
					"id": "obj-23",
					"maxclass": "message",
					"patching_rect": [
						560.0,
						360.0,
						70.0,
						22.0
					],
					"text": "eval 1+1",
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
						635.0,
						360.0,
						20.0,
						20.0
					],
					"text": "c",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-25",
					"maxclass": "newobj",
					"patching_rect": [
						700.0,
						360.0,
						60.0,
						22.0
					],
					"text": "counter",
					"numinlets": 3,
					"numoutlets": 4,
					"outlettype": [
						"int",
						"",
						"",
						"int"
					]
				}
			},
			{
				"box": {
					"id": "obj-26",
					"maxclass": "newobj",
					"patching_rect": [
						460.0,
						485.0,
						60.0,
						22.0
					],
					"text": "counter",
					"numinlets": 3,
					"numoutlets": 4,
					"outlettype": [
						"int",
						"",
						"",
						"int"
					]
				}
			},
			{
				"box": {
					"id": "obj-27",
					"maxclass": "newobj",
					"patching_rect": [
						460.0,
						455.0,
						30.0,
						22.0
					],
					"text": "t b",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "obj-28",
					"maxclass": "number",
					"patching_rect": [
						700.0,
						390.0,
						60.0,
						22.0
					],
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-29",
					"maxclass": "number",
					"patching_rect": [
						460.0,
						515.0,
						60.0,
						22.0
					],
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-30",
					"maxclass": "comment",
					"patching_rect": [
						765.0,
						390.0,
						60.0,
						20.0
					],
					"text": "sent",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-31",
					"maxclass": "comment",
					"patching_rect": [
						525.0,
						515.0,
						60.0,
						20.0
					],
					"text": "done",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-32",
					"maxclass": "newobj",
					"patching_rect": [
						700.0,
						300.0,
						70.0,
						22.0
					],
					"text": "metro 100",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "obj-33",
					"maxclass": "newobj",
					"patching_rect": [
						800.0,
						330.0,
						60.0,
						22.0
					],
					"text": "counter",
					"numinlets": 3,
					"numoutlets": 4,
					"outlettype": [
						"int",
						"",
						"",
						"int"
					]
				}
			},
			{
				"box": {
					"id": "obj-34",
					"maxclass": "number",
					"patching_rect": [
						800.0,
						360.0,
						60.0,
						22.0
					],
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"",
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-35",
					"maxclass": "comment",
					"patching_rect": [
						865.0,
						360.0,
						80.0,
						20.0
					],
					"text": "heartbeat",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-36",
					"maxclass": "comment",
					"patching_rect": [
						20.0,
						600.0,
						420.0,
						80.0
					],
					"text": "3. Outlet release (Overdrive off; run 1a and 1b first)\nClick e. Max freezes for about a second while [uzi] runs.\ngil_thread prints 1, then the tick rate during the outlet call.\nOK: near 100. BUG: near 0 (the outlet held the GIL).",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 4
				}
			},
			{
				"box": {
					"id": "obj-37",
					"maxclass": "button",
					"patching_rect": [
						20.0,
						690.0,
						24.0,
						24.0
					],
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					],
					"parameter_enable": 0
				}
			},
			{
				"box": {
					"id": "obj-38",
					"maxclass": "comment",
					"patching_rect": [
						50.0,
						690.0,
						20.0,
						20.0
					],
					"text": "e",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-39",
					"maxclass": "newobj",
					"patching_rect": [
						20.0,
						725.0,
						60.0,
						22.0
					],
					"text": "t b b b",
					"numinlets": 1,
					"numoutlets": 3,
					"outlettype": [
						"bang",
						"bang",
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "obj-40",
					"maxclass": "message",
					"patching_rect": [
						200.0,
						760.0,
						90.0,
						22.0
					],
					"text": "eval mark()",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-41",
					"maxclass": "message",
					"patching_rect": [
						110.0,
						760.0,
						70.0,
						22.0
					],
					"text": "eval 1+1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-42",
					"maxclass": "message",
					"patching_rect": [
						20.0,
						790.0,
						160.0,
						22.0
					],
					"text": "eval rate_since_mark()",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-43",
					"maxclass": "newobj",
					"patching_rect": [
						110.0,
						820.0,
						130.0,
						22.0
					],
					"text": "py @name gil_out",
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
					"id": "obj-44",
					"maxclass": "newobj",
					"patching_rect": [
						110.0,
						850.0,
						30.0,
						22.0
					],
					"text": "t b",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "obj-45",
					"maxclass": "newobj",
					"patching_rect": [
						110.0,
						880.0,
						100.0,
						22.0
					],
					"text": "uzi 5000000",
					"numinlets": 2,
					"numoutlets": 3,
					"outlettype": [
						"bang",
						"bang",
						"int"
					]
				}
			},
			{
				"box": {
					"id": "obj-46",
					"maxclass": "newobj",
					"patching_rect": [
						110.0,
						910.0,
						60.0,
						22.0
					],
					"text": "counter",
					"numinlets": 3,
					"numoutlets": 4,
					"outlettype": [
						"int",
						"",
						"",
						"int"
					]
				}
			},
			{
				"box": {
					"id": "obj-47",
					"maxclass": "comment",
					"patching_rect": [
						460.0,
						600.0,
						460.0,
						60.0
					],
					"text": "4. Re-entry (either Overdrive setting)\nf: py outputs 2, [sel 2] sends eval 3+3 back into the same py.\nOK: gil_re prints 6 and 2, with no error or hang.",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 3
				}
			},
			{
				"box": {
					"id": "obj-48",
					"maxclass": "message",
					"patching_rect": [
						460.0,
						660.0,
						70.0,
						22.0
					],
					"text": "eval 1+1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-49",
					"maxclass": "comment",
					"patching_rect": [
						535.0,
						660.0,
						20.0,
						20.0
					],
					"text": "f",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-50",
					"maxclass": "newobj",
					"patching_rect": [
						460.0,
						700.0,
						130.0,
						22.0
					],
					"text": "py @name gil_re",
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
					"id": "obj-51",
					"maxclass": "newobj",
					"patching_rect": [
						460.0,
						735.0,
						50.0,
						22.0
					],
					"text": "sel 2",
					"numinlets": 2,
					"numoutlets": 2,
					"outlettype": [
						"bang",
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-52",
					"maxclass": "message",
					"patching_rect": [
						460.0,
						765.0,
						70.0,
						22.0
					],
					"text": "eval 3+3",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-53",
					"maxclass": "newobj",
					"patching_rect": [
						600.0,
						735.0,
						80.0,
						22.0
					],
					"text": "print gil_re",
					"numinlets": 1,
					"numoutlets": 0,
					"outlettype": []
				}
			},
			{
				"box": {
					"id": "obj-54",
					"maxclass": "comment",
					"patching_rect": [
						460.0,
						810.0,
						460.0,
						40.0
					],
					"text": "5. api outlets: g, then each of h to l\nOK: gil_api prints 7, 1.5, hi, 1 2, then bang, each without error.",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 2
				}
			},
			{
				"box": {
					"id": "obj-55",
					"maxclass": "newobj",
					"patching_rect": [
						460.0,
						990.0,
						130.0,
						22.0
					],
					"text": "py @name gil_api",
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
					"id": "obj-56",
					"maxclass": "newobj",
					"patching_rect": [
						460.0,
						1020.0,
						90.0,
						22.0
					],
					"text": "print gil_api",
					"numinlets": 1,
					"numoutlets": 0,
					"outlettype": []
				}
			},
			{
				"box": {
					"id": "obj-57",
					"maxclass": "message",
					"patching_rect": [
						460.0,
						850.0,
						250.0,
						22.0
					],
					"text": "exec import api",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-58",
					"maxclass": "comment",
					"patching_rect": [
						720.0,
						850.0,
						20.0,
						20.0
					],
					"text": "g",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-59",
					"maxclass": "message",
					"patching_rect": [
						460.0,
						872.0,
						250.0,
						22.0
					],
					"text": "exec api.PyExternal().out(7)",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-60",
					"maxclass": "comment",
					"patching_rect": [
						720.0,
						872.0,
						20.0,
						20.0
					],
					"text": "h",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-61",
					"maxclass": "message",
					"patching_rect": [
						460.0,
						894.0,
						250.0,
						22.0
					],
					"text": "exec api.PyExternal().out(1.5)",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-62",
					"maxclass": "comment",
					"patching_rect": [
						720.0,
						894.0,
						20.0,
						20.0
					],
					"text": "i",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-63",
					"maxclass": "message",
					"patching_rect": [
						460.0,
						916.0,
						250.0,
						22.0
					],
					"text": "exec \"api.PyExternal().out('hi')\"",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-64",
					"maxclass": "comment",
					"patching_rect": [
						720.0,
						916.0,
						20.0,
						20.0
					],
					"text": "j",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-65",
					"maxclass": "message",
					"patching_rect": [
						460.0,
						938.0,
						250.0,
						22.0
					],
					"text": "exec \"api.PyExternal().out([1, 2])\"",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-66",
					"maxclass": "comment",
					"patching_rect": [
						720.0,
						938.0,
						20.0,
						20.0
					],
					"text": "k",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 1
				}
			},
			{
				"box": {
					"id": "obj-67",
					"maxclass": "message",
					"patching_rect": [
						460.0,
						960.0,
						250.0,
						22.0
					],
					"text": "exec api.PyExternal().bang()",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-68",
					"maxclass": "comment",
					"patching_rect": [
						720.0,
						960.0,
						20.0,
						20.0
					],
					"text": "l",
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
						"obj-4",
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
						"obj-3",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-6",
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
			},
			{
				"patchline": {
					"source": [
						"obj-8",
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
						"obj-17",
						0
					],
					"destination": [
						"obj-19",
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
						"obj-32",
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
						"obj-20",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-20",
						1
					],
					"destination": [
						"obj-25",
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
						"obj-28",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-20",
						0
					],
					"destination": [
						"obj-21",
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
						"obj-22",
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
						"obj-22",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-22",
						0
					],
					"destination": [
						"obj-27",
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
						"obj-26",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-26",
						0
					],
					"destination": [
						"obj-29",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-32",
						0
					],
					"destination": [
						"obj-33",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-33",
						0
					],
					"destination": [
						"obj-34",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-37",
						0
					],
					"destination": [
						"obj-39",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-39",
						2
					],
					"destination": [
						"obj-40",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-39",
						1
					],
					"destination": [
						"obj-41",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-39",
						0
					],
					"destination": [
						"obj-42",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-40",
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
						"obj-42",
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
						"obj-41",
						0
					],
					"destination": [
						"obj-43",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-43",
						0
					],
					"destination": [
						"obj-44",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-44",
						0
					],
					"destination": [
						"obj-45",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-45",
						0
					],
					"destination": [
						"obj-46",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-48",
						0
					],
					"destination": [
						"obj-50",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-50",
						0
					],
					"destination": [
						"obj-51",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-51",
						0
					],
					"destination": [
						"obj-52",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-52",
						0
					],
					"destination": [
						"obj-50",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-50",
						0
					],
					"destination": [
						"obj-53",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-55",
						0
					],
					"destination": [
						"obj-56",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-57",
						0
					],
					"destination": [
						"obj-55",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-59",
						0
					],
					"destination": [
						"obj-55",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-61",
						0
					],
					"destination": [
						"obj-55",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-63",
						0
					],
					"destination": [
						"obj-55",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-65",
						0
					],
					"destination": [
						"obj-55",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-67",
						0
					],
					"destination": [
						"obj-55",
						0
					]
				}
			}
		]
	}
}
