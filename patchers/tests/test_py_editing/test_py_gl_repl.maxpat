{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 8,
			"minor" : 6,
			"revision" : 2,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 443.0, 145.0, 640.0, 480.0 ],
		"bglocked" : 0,
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 1,
		"objectsnaponopen" : 1,
		"statusbarvisible" : 2,
		"toolbarvisible" : 1,
		"lefttoolbarpinned" : 0,
		"toptoolbarpinned" : 0,
		"righttoolbarpinned" : 0,
		"bottomtoolbarpinned" : 0,
		"toolbars_unpinned_last_save" : 0,
		"tallnewobj" : 0,
		"boxanimatetime" : 200,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"description" : "",
		"digest" : "",
		"tags" : "",
		"style" : "",
		"subpatcher_template" : "",
		"assistshowspatchername" : 0,
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-6",
					"linecount" : 3,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 177.0, 342.0, 327.0, 47.0 ],
					"text" : "option-return to evaluate the contents of the repl\noption-G to switch on epemeral mode (i.e. clear on return\noption-z to clear the repl"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-3",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 174.0, 388.0, 330.0, 20.0 ],
					"text" : "requires this package: https://github.com/twhiston/tw.gl.repl"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"id" : "obj-45",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"outlinecolor" : [ 0.92549, 0.364706, 0.341176, 1.0 ],
					"parameter_enable" : 0,
					"patching_rect" : [ 327.0, 279.0, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"id" : "obj-46",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"outlinecolor" : [ 0.0, 0.533333, 0.168627, 1.0 ],
					"parameter_enable" : 0,
					"patching_rect" : [ 371.0, 279.0, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-20",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 175.0, 241.0, 29.5, 22.0 ],
					"text" : "set"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-18",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 175.0, 196.0, 58.0, 22.0 ],
					"text" : "loadbang"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-15",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 175.0, 161.0, 85.0, 20.0 ],
					"text" : "toggle this =>"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-13",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 338.0, 160.0, 35.0, 22.0 ],
					"text" : "clear"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-11",
					"maxclass" : "toggle",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "int" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 283.0, 159.0, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 8,
							"minor" : 6,
							"revision" : 2,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"classnamespace" : "box",
						"rect" : [ 96.0, 106.0, 640.0, 480.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 0,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"assistshowspatchername" : 0,
						"boxes" : [ 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-6",
									"index" : 1,
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "int" ],
									"patching_rect" : [ 110.0, 28.0, 30.0, 30.0 ]
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-5",
									"index" : 2,
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 387.0, 47.0, 30.0, 30.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-9",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 343.0, 222.0, 83.0, 22.0 ],
									"text" : "ignore_keys 0"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-1",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 50.0, 226.0, 79.0, 22.0 ],
									"text" : "loadmess init"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-80",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 253.0, 222.0, 86.0, 22.0 ],
									"text" : "routepass size"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-79",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 225.0, 153.0, 47.0, 22.0 ],
									"text" : "getsize"
								}

							}
, 							{
								"box" : 								{
									"attr" : "visible",
									"id" : "obj-77",
									"maxclass" : "attrui",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 50.0, 100.0, 150.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-75",
									"linecount" : 2,
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "" ],
									"patching_rect" : [ 50.0, 265.0, 376.0, 35.0 ],
									"text" : "tw.gl.repl py-repl 1280 720 @cursor | @font \"Andale Mono\" @comment # @blink_enable 1 @blink_time 250"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-74",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 3,
									"outlettype" : [ "jit_matrix", "bang", "" ],
									"patching_rect" : [ 50.0, 192.0, 210.0, 22.0 ],
									"text" : "jit.world py-repl @enable 0 @visible 0"
								}

							}
, 							{
								"box" : 								{
									"attr" : "enable",
									"id" : "obj-76",
									"maxclass" : "attrui",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 50.0, 131.0, 150.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-3",
									"index" : 1,
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 50.0, 360.0, 30.0, 30.0 ]
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-75", 0 ],
									"source" : [ "obj-1", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-75", 0 ],
									"source" : [ "obj-5", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-76", 0 ],
									"order" : 0,
									"source" : [ "obj-6", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-77", 0 ],
									"order" : 1,
									"source" : [ "obj-6", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-75", 0 ],
									"midpoints" : [ 155.0, 251.0, 59.5, 251.0 ],
									"order" : 1,
									"source" : [ "obj-74", 1 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-79", 0 ],
									"midpoints" : [ 155.0, 218.0, 283.75, 218.0, 283.75, 142.0, 234.5, 142.0 ],
									"order" : 0,
									"source" : [ "obj-74", 1 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-80", 0 ],
									"source" : [ "obj-74", 2 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-3", 0 ],
									"source" : [ "obj-75", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-74", 0 ],
									"source" : [ "obj-76", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-74", 0 ],
									"source" : [ "obj-77", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-74", 0 ],
									"source" : [ "obj-79", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-75", 0 ],
									"source" : [ "obj-80", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-75", 0 ],
									"source" : [ "obj-9", 0 ]
								}

							}
 ]
					}
,
					"patching_rect" : [ 283.0, 196.0, 74.0, 22.0 ],
					"saved_object_attributes" : 					{
						"description" : "",
						"digest" : "",
						"globalpatchername" : "",
						"tags" : ""
					}
,
					"text" : "p py_gl_repl"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-8",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 175.0, 279.0, 127.0, 22.0 ],
					"text" : "400"
				}

			}
, 			{
				"box" : 				{
					"autoload" : 0,
					"file" : "",
					"id" : "obj-2",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "", "bang", "bang" ],
					"patching_rect" : [ 283.0, 241.0, 107.0, 22.0 ],
					"saved_object_attributes" : 					{
						"autoload" : 0,
						"file" : "",
						"pythonpath" : "/"
					}
,
					"text" : "py",
					"varname" : "__main__"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-5", 0 ],
					"source" : [ "obj-11", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-5", 1 ],
					"source" : [ "obj-13", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-20", 0 ],
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-45", 0 ],
					"source" : [ "obj-2", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-46", 0 ],
					"source" : [ "obj-2", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-8", 1 ],
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-8", 0 ],
					"source" : [ "obj-20", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-5", 0 ]
				}

			}
 ],
		"dependency_cache" : [ 			{
				"name" : "BraceBalancedFormatter.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/TextBuffer/formatters",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/TextBuffer/formatters",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "CommentRemoverFormatter.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/TextBuffer/formatters",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/TextBuffer/formatters",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "Cursor.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/Cursor",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/Cursor",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "GLRender.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/GLRender",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/GLRender",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "KeypressProcessor.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/KeypressProcessor",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/KeypressProcessor",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "MaxBindings.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/MaxBindings",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/MaxBindings",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "REPLManager.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/REPLManager",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/REPLManager",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "SingleLineOutputFormatter.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/TextBuffer/formatters",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/TextBuffer/formatters",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "TextBuffer.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/TextBuffer",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/TextBuffer",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "WhitespaceFormatter.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/TextBuffer/formatters",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/TextBuffer/formatters",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "array.extensions.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/extensions",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/extensions",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "includes.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "object.extensions.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/extensions",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/extensions",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "patcher-init.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "py.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "string.extensions.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript/extensions",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript/extensions",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "tw.gl.repl.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/javascript",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/javascript",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "tw.gl.repl.maxpat",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/patchers",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/patchers",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "user-repl.js",
				"bootpath" : "~/Documents/Max 8/Packages/GLRepl/examples/custom-formatter",
				"patcherrelativepath" : "../../../../../../Documents/Max 8/Packages/GLRepl/examples/custom-formatter",
				"type" : "TEXT",
				"implicit" : 1
			}
 ],
		"autosave" : 0
	}

}
