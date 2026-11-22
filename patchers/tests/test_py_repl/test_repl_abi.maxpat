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
			600.0,
			240.0
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
						560,
						60.0
					],
					"text": "py_repl interface (A_CANT repl methods), called through ctypes.\nClick a, then b. Expect 'repl_tester: 14/14 ok'. Failures are posted as errors.\nThe interrupt test runs a 10 s loop and stops it after 0.3 s: Max freezes briefly.",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 3
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "message",
					"patching_rect": [
						20,
						80,
						150,
						22.0
					],
					"text": "load test_repl_abi.py",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-3",
					"maxclass": "comment",
					"patching_rect": [
						175,
						80,
						20,
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
					"id": "obj-4",
					"maxclass": "message",
					"patching_rect": [
						20,
						110,
						110,
						22.0
					],
					"text": "eval run_all()",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-5",
					"maxclass": "comment",
					"patching_rect": [
						135,
						110,
						20,
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
					"id": "obj-6",
					"maxclass": "newobj",
					"patching_rect": [
						20,
						150,
						130,
						22.0
					],
					"text": "py @name repl_tester",
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
					"id": "obj-7",
					"maxclass": "newobj",
					"patching_rect": [
						20,
						185,
						110,
						22.0
					],
					"text": "print repl_tester",
					"numinlets": 1,
					"numoutlets": 0
				}
			},
			{
				"box": {
					"id": "obj-8",
					"maxclass": "newobj",
					"patching_rect": [
						300,
						150,
						130,
						22.0
					],
					"text": "py @name repl_target",
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
					"id": "obj-9",
					"maxclass": "comment",
					"patching_rect": [
						300,
						180,
						260,
						20.0
					],
					"text": "the target: runs the test code in its namespace",
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
						"obj-2",
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
						"obj-6",
						0
					],
					"destination": [
						"obj-7",
						0
					]
				}
			}
		]
	}
}
