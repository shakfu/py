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
			580.0
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
			}
		]
	}
}
