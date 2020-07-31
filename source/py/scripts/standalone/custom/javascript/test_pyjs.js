var pyjs = new PyJS();

// ---------------------------------------------------------------------------
// Properties

function set_name(val)
{
	pyjs.name = val;
}

function get_name()
{
	outlet(0, pyjs.name);
}

function set_file(val)
{
	pyjs.file = val;
}

function get_file()
{
	outlet(0, pyjs.file);
}

function set_pythonpath(val)
{
	pyjs.pythonpath = val;
}

function get_pythonpath()
{
	outlet(0, pyjs.pythonpath);
}

function set_debug(val)
{
	pyjs.debug = val;
}

function get_debug()
{
	outlet(0, pyjs.debug);
}

// ---------------------------------------------------------------------------
// Methods

function test_patcher_message()
{
	var p = this.patcher;

	// message to named button
	var mr = p.getnamed("mrbang");
	mr.message("bang");

	// message to self
	var me = p.getnamed("bob"); // bob is a scripting name
	me.message("py", Array("from", "sys", "import", "version")); // works
	var res = pyjs.eval("version");
	if (res)
		outlet(0, res);
}

function test_messnamed()
{
	// only to receive objects
	messnamed("mrbang", "bang");
}

function test_code_array_req()
{
	//pyjs.code("from sys import version");  // nope
	pyjs.code("from sys import version".split(" ")); // works
	var res = pyjs.code("version");
	outlet(0, res);		
}

function test_exec_eval()
{
	pyjs.exec("from random import random");
	var res = pyjs.eval("random()");
	if (res)
		outlet(0, res);
}


function execfile(file)
{
	pyjs.execfile(file);
}


function test_execfile()
{
	pyjs.execfile("hello.py");
	var res = pyjs.eval("c");
	if (res)
		outlet(0, res); // should output "Hello World"
}

// in-js wrapper for pyjs_code
// (allows both eval and exec in js code)
function pyc(str)
{
	return pyjs.code(str.split(" "));
}


function test_pyc()
{
	pyc("from sys import version");
	var res = pyc("version");
	outlet(0, res);
}

// external use wrapper for pyjs_code
// for messages (and evals) only
function py()
{
	var arr = arrayfromargs(arguments);
	// post(arr + "\n");
	var res = pyjs.code(arr)
	if (res)
		outlet(0, res);
}


function test_json()
{
	pyc("import json");
	var res = pyc("json.dumps(list(range(5)))")
	var json = JSON.parse(res);
	post(json+'\n');
	var res = pyc("json.dumps(dict(a=12131))")
	var obj = JSON.parse(res);
	post(obj.a+'\n');
	
}

// TOFIX!
// WARNING! Reliably causes crash!!
function test_eval_to_json()
{
	var res = pyjs.eval_to_json("list(range(5))");
	var json = JSON.parse(res);
	post(json+'\n');
}


