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
        "rect": [ 102.0, 111.0, 666.0, 464.0 ],
        "boxes": [
            {
                "box": {
                    "fontface": 1,
                    "fontsize": 16.0,
                    "id": "obj-15",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 64.0, 8.0, 257.0, 24.0 ],
                    "text": "py_code: a python editor in Max"
                }
            },
            {
                "box": {
                    "id": "obj-12",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 64.0, 412.0, 48.0, 22.0 ],
                    "text": "print py"
                }
            },
            {
                "box": {
                    "id": "obj-9",
                    "maxclass": "button",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "bang" ],
                    "outlinecolor": [ 0.439216, 0.74902, 0.254902, 1.0 ],
                    "parameter_enable": 0,
                    "patching_rect": [ 134.0, 378.0, 24.0, 24.0 ]
                }
            },
            {
                "box": {
                    "id": "obj-7",
                    "maxclass": "button",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "bang" ],
                    "outlinecolor": [ 0.92549, 0.364706, 0.341176, 1.0 ],
                    "parameter_enable": 0,
                    "patching_rect": [ 99.0, 378.0, 24.0, 24.0 ]
                }
            },
            {
                "box": {
                    "autoload": 0,
                    "file": "",
                    "id": "obj-5",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 3,
                    "outlettype": [ "", "bang", "bang" ],
                    "patching_rect": [ 64.0, 349.0, 89.0, 22.0 ],
                    "saved_object_attributes": {
                        "autoload": 0,
                        "file": "",
                        "pythonpath": ""
                    },
                    "text": "py",
                    "varname": "__main__"
                }
            },
            {
                "box": {
                    "id": "obj-1",
                    "maxclass": "py_code",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 64.0, 56.0, 459.0, 274.0 ],
                    "py_code_caret": 0,
                    "py_code_scroll": 0,
                    "py_code_text": "",
                    "snippets": ""
                }
            }
        ],
        "lines": [
            {
                "patchline": {
                    "destination": [ "obj-5", 0 ],
                    "source": [ "obj-1", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-12", 0 ],
                    "source": [ "obj-5", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-7", 0 ],
                    "source": [ "obj-5", 1 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-9", 0 ],
                    "source": [ "obj-5", 2 ]
                }
            }
        ],
        "autosave": 0
    }
}