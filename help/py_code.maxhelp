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
                    "attr": "theme",
                    "id": "obj-2",
                    "maxclass": "attrui",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "parameter_enable": 0,
                    "patching_rect": [ 64.0, 49.0, 459.0, 22.0 ]
                }
            },
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
                    "patching_rect": [ 64.0, 431.0, 48.0, 22.0 ],
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
                    "patching_rect": [ 134.0, 397.0, 24.0, 24.0 ]
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
                    "patching_rect": [ 99.0, 397.0, 24.0, 24.0 ]
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
                    "patching_rect": [ 64.0, 368.0, 89.0, 22.0 ],
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
                    "barcolor": [ 0.12941176470588237, 0.1450980392156863, 0.16862745098039217, 1.0 ],
                    "barhighlight": [ 0.3803921568627451, 0.6862745098039216, 0.9372549019607843, 1.0 ],
                    "bgcolor": [ 0.1568627450980392, 0.17254901960784313, 0.20392156862745098, 1.0 ],
                    "blinkcolor": [ 0.3803921568627451, 0.6862745098039216, 0.9372549019607843, 0.4 ],
                    "bracketcolor": [ 0.4980392156862745, 0.5176470588235295, 0.5568627450980392, 1.0 ],
                    "builtincolor": [ 0.33725490196078434, 0.7137254901960784, 0.7607843137254902, 1.0 ],
                    "caretcolor": [ 0.3215686274509804, 0.5450980392156862, 1.0, 1.0 ],
                    "classcolor": [ 0.8980392156862745, 0.7529411764705882, 0.4823529411764706, 1.0 ],
                    "commentcolor": [ 0.3607843137254902, 0.38823529411764707, 0.4392156862745098, 1.0 ],
                    "curlinecolor": [ 0.17254901960784313, 0.19215686274509805, 0.22745098039215686, 1.0 ],
                    "decoratorcolor": [ 0.8196078431372549, 0.6039215686274509, 0.4, 1.0 ],
                    "errorcolor": [ 0.8784313725490196, 0.4235294117647059, 0.4588235294117647, 1.0 ],
                    "escapecolor": [ 0.596078431372549, 0.7647058823529411, 0.4745098039215686, 1.0 ],
                    "file": "",
                    "functioncolor": [ 0.8784313725490196, 0.4235294117647059, 0.4588235294117647, 1.0 ],
                    "id": "obj-1",
                    "keywordcolor": [ 0.7764705882352941, 0.47058823529411764, 0.8666666666666667, 1.0 ],
                    "matchcolor": [ 0.3803921568627451, 0.6862745098039216, 0.9372549019607843, 0.3 ],
                    "maxclass": "py_code",
                    "numbercolor": [ 0.8196078431372549, 0.6039215686274509, 0.4, 1.0 ],
                    "numinlets": 1,
                    "numoutlets": 1,
                    "operatorcolor": [ 0.6705882352941176, 0.6980392156862745, 0.7490196078431373, 1.0 ],
                    "outlettype": [ "" ],
                    "patching_rect": [ 64.0, 81.0, 459.0, 274.0 ],
                    "py_code_caret": 0,
                    "py_code_scroll": 0.0,
                    "py_code_text": "",
                    "selcolor": [ 0.24313725490196078, 0.26666666666666666, 0.3176470588235294, 1.0 ],
                    "selfcolor": [ 0.8784313725490196, 0.4235294117647059, 0.4588235294117647, 1.0 ],
                    "snippets": "",
                    "stringcolor": [ 0.596078431372549, 0.7647058823529411, 0.4745098039215686, 1.0 ],
                    "textcolor": [ 0.6705882352941176, 0.6980392156862745, 0.7490196078431373, 1.0 ],
                    "warningcolor": [ 0.8980392156862745, 0.7529411764705882, 0.4823529411764706, 1.0 ]
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
                    "destination": [ "obj-1", 0 ],
                    "source": [ "obj-2", 0 ]
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