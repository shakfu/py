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
			660.0,
			250.0
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
						600,
						90.0
					],
					"text": "Interrupt key probe for py_repl. Click a, then b: Max freezes for 5 s.\nWhile frozen: press Ctrl+C once in Max, then Cmd+Tab to another app and press it once.\nExpect: 'ctrl-c presses: 1 with Max in front, 0 with another app in front':\nwithout Input Monitoring, macOS does not report keys pressed in other apps.\n0 and 0 means key polling does not work without a permission (check the console too).",
					"numinlets": 1,
					"numoutlets": 0,
					"linecount": 5
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "message",
					"patching_rect": [
						20,
						100,
						140,
						22.0
					],
					"text": "load probe_keys.py",
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
						165,
						100,
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
						130,
						100,
						22.0
					],
					"text": "eval probe(5)",
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
						125,
						130,
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
						170,
						130,
						22.0
					],
					"text": "py @name keys_probe",
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
						205,
						110,
						22.0
					],
					"text": "print keys_probe",
					"numinlets": 1,
					"numoutlets": 0
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
