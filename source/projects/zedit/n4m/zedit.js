
const fs = require('fs/promises');

const express = require("express");
const app = express();
const Max = require("max-api");

const PORT = 3000;

async function write_to_file(path, content) {
 	try {
    	await fs.writeFile(path, content);
  	} catch (err) {
    	console.log(err);
  	}
}


function anypost(str) {
	if (Max) {
		Max.post(str);
	} else {
		console.log(str);
	}
}

app.use(express.static('public'))
app.use(express.json());

// app.get("/", function (req, res) {
// 	res.send("<p>Hello World!<p>");
// });

// app.get("/editor/help", function (req, res) {
// 	res.send("<p>Hello World!<p>");
// });

app.post('/api/code/save', (req, res) => {
 	// res.send('Hello World!')
 	anypost("/api/code/save called");
 	console.log(req.body);
})

app.post('/api/code/run', (req, res) => {
 	// res.send('Hello World!')
 	anypost("/api/code/run called");
 	console.log(req.body);
})



// app.listen(PORT, () => console.log(`Server listening on port: ${PORT}`));

app.listen(PORT, function () {
	anypost(`Example app listening on port ${PORT}!"`);
	if (Max) Max.outlet("ready");
});

