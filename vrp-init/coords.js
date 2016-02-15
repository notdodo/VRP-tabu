var i = 1;
var canvasHTML = document.getElementById('coords'), canvas = new fabric.Canvas('coords');
var rect = canvasHTML.getBoundingClientRect();
var clicks = document.getElementById('clicks');
var currentPos = document.getElementById('coord_msg');
var fileupload = document.getElementById('fileupload');
var loadedPrinted = false;
var routeDiv = document.getElementById('routes');
var pars = $("input[type='number'");
// vertex and routes array
var vertex = [], routes = [], canvasCustomers = [], costs = [];
var centerX = canvas.width / 2 - 6, centerY = canvas.height / 2 - 2;
var fontSize = 14;
var intRegex = /(\d+)/g;
var flagVRP = false;

/* display the center of the canvas */
$(function() {
    document.getElementById("play").checked = false;
    document.getElementById("play").value = 0;
    clicks.innerHTML = "<h2>Customers</h2>";
    // Depot
    canvas.add(new fabric.Text('☐', {
        fontFamily: 'Roboto',
        fontSize: fontSize,
        fill: 'black',
        left: (centerX - 5),
        top: (centerY - 7),
        selectable: false
    }));
    canvas.add(new fabric.Text('(0;0)', {
        fontFamily: 'sans-serif',
        fontSize: fontSize+2,
        fill: 'white',
        left: (centerX - 18),
        top: (centerY + 8),
        selectable: false,
        uid: "depot"
    }));

    // item of vertex array
    var item = {
            name: "V0",
            x: 0,
            y: 0
    };
    vertex.push(item);

    for (var n in pars)
        pars[n].disabled = false;
    // open a file
    fileupload.addEventListener('change', readSingleFile, false);
    // on mouse move event
    canvas.on('mouse:move', evt => {
        var mousePos = getMousePos(evt.e);
        var message = parseInt(mousePos.x) + ', ' + parseInt(mousePos.y);
        currentPos.innerHTML = message;
    }, false);

    // on mouse click event
    canvas.on('mouse:down', e => {
        var move = false;
        canvas.on('object:moving', options => {
            move = true;
            var idCust = parseInt(options.target.uid.match(intRegex));
            var mousePos = getMousePos(options.e);
            vertex[idCust].x = mousePos.x;
            vertex[idCust].y = mousePos.y;
        }, false);
        canvas.on('mouse:up', evt => {
            if(!move) {
                var mousePos = getMousePos(evt.e);
                var x = mousePos.x, y = mousePos.y;
                var id = "<span class='num'>" + "V<sub>" + i + "</sub>" + "</span> ";
                var cust = id + $.strPad(x, 4) + "; " + $.strPad(y, 4) + "<br/>";
                clicks.innerHTML += cust;
                drawCustomer(i, evt.e);
                if (parseInt(i, 10) < 10)
                    var n = "V0" + i;
                else
                    var n = "V" + i;
                var item = {
                    name: n,
                    x: x,
                    y: y
                };
                vertex.push(item);
                i++;
            }
            move = true;
        }, false);
    }, false);
});

function playStop(c) {
    if(c.value == 1) {
        loadedPrinted = false;
        c.value = 0;
        c.checked = false;
    }else if(c.value == 0) {
        loadedPrinted = true;
        c.value = 1;
        c.checked = true;
        loadJSON();
    }
}

function loadJSON() {
    if (loadedPrinted) {
        var xobj = new XMLHttpRequest();
        xobj.overrideMimeType("application/json");
        xobj.open('GET', 'output.json', true);
        xobj.onreadystatechange = function () {
            if (xobj.readyState == 4 && xobj.status == "200") {
                parseCustomer(JSON.parse(xobj.responseText));
            }
        };
        xobj.send(null);
    }
 }

var blob = new Blob(["self.onmessage = function(event) { postMessage(event.data); }"], {type: 'application/javascript'});
var worker = new Worker(URL.createObjectURL(blob));
worker.onmessage = function(event) {
    loadJSON();
};
function runWorker() {
    worker.postMessage("run");
}
window.setInterval(runWorker, 4000);

/* get the mouse position on the canvas */
function getMousePos(evt) {
    return {
        x: (evt.clientX - rect.left) - canvasHTML.width / 2,
        y: -((evt.clientY - rect.top) - canvasHTML.height / 2)
    };
}

/* onclick draw a customer */
function drawCustomer(uid, evt, drag) {
	if (typeof(drag) === 'undefined') drag = true;
    var x = evt.clientX - rect.left, y = evt.clientY - rect.top;
    var last = new fabric.Text('⌂', {
        fontFamily: 'Roboto',
        fontSize: fontSize,
        fill: '#CF000F',
        left: (x - 6),
        top: (y - 10),
        hasBorders: false,
        hasControls: false,
        hasRotatingPoint: false,
        selectable: drag,
        targetFindTolerance: 5,
        uid: "C" + uid
    });
    canvas.add(last);
    canvasCustomers[uid] = last;
}
//pads left
$.strPad = (s, l) => {
    var o = s.toString();
    var dif = l - o.length;
    if (i <= 9)
        dif++;
    o = Array(dif+1).join("&nbsp;") + s;
    return o;
};

/* get a random int from min to max */
function getRandomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

/* add request and service time per each customer */
function setRequestTime(par) {
    var arr = [];
    for (var k = 1; k < i; k++) {
        vertex[k].request = getRandomInt(1, par[4].value);
        vertex[k].time = getRandomInt(1, par[5].value);
    }
}

/* generate the triangular matrix of the arcs cost with null diagonal*/
function getCostsMatrix(max) {
    var costs = [];
    for (var k = 0; k < vertex.length; k++) {
        var row = [];
        for (j = 0; j < vertex.length; j++) {
            if (k !== j) {
                var item = {
                    from : k,
                    to: j,
                    value: Math.round(distance(vertex[k], vertex[j]))
                };
                row.push(item);
            }
        }
        costs.push(row);
    }
    return costs;
}

function distance(a, b) {
    var dist = Math.sqrt(Math.pow(a.x - b.x, 2) + Math.pow(a.y - b.y, 2));
    return dist;
}

/* generate the json file per the program */
function saveToFile() {
    setRequestTime(pars);
    if (vertex.length > 1) {
        outputJSON = JSON.stringify({
            vertices: vertex,
            "vehicles": parseInt(pars[0].value, 10),
            "capacity": parseInt(pars[1].value, 10),
            "worktime": parseInt(pars[2].value, 10),
            "costs": getCostsMatrix(pars[3].value)
        }, null, 4);
        createFile(outputJSON);
    }else
        alert("Please insert some customers");
}

function createFile(data) {
    var toSave = new Blob([data], {
        type: 'text/plain'
    });
    var textFile = null;
    if (textFile !== null) {
        window.URL.revokeObjectURL(textFile);
    }
    textFile = window.URL.createObjectURL(toSave);
    var link = document.getElementById("download");
    link.href = textFile;
    link.click();
}

function readSingleFile(evt) {
    canvas.__eventListeners["mouse:down"] = [];
	canvas.__eventListeners["mouse:over"] = [];
	canvas.__eventListeners["mouse:out"] = [];
	routeDiv.innerHTML = "";
    //Retrieve the first (and only!) File from the FileList object
    var f = evt.target.files[0];
    var time = f.lastModifiedDate;

    if (f) {
        var r = new FileReader();
        r.onload = function(e) {
            var contents = e.target.result;
			var extension = f.name.substr((~-f.name.lastIndexOf(".") >>> 0) + 2);
            if (f.type == "application/json" && extension === "json") {
                var data = JSON.parse(contents);
                parseCustomer(data);
            }else if (f.type === "" && extension === "vrp") {
				convertFile(contents);
			}
        };
        r.onerror = function(e) {
            alert("Errore!");
        };
        r.readAsText(f);
    } else {
        alert("Failed to load file");
    }
}


/* parse the json and display the results */
/* ~130 microseconds */
function parseCustomer(cs) {
    canvas.__eventListeners["mouse:down"] = [];
    canvas.__eventListeners["mouse:over"] = [];
    canvas.__eventListeners["mouse:out"] = [];
    routeDiv.innerHTML = "";
    var start = performance.now();
    if (cs.type === "VRP")
        flagVRP = true;
    if (Array.isArray(cs.vertices)) {
        // before draw clean all the canvas
        canvas.on('mouse:over', onMouse);
        canvas.on('mouse:out', onMouse);
        clicks.innerHTML = "<h2>Customers</h2>";
        for (var e in canvasCustomers)
            canvas.remove(canvasCustomers[e]);
        for (var r in routes)
            canvas.remove(routes[r].path);
        vertex = [];
        routes = [];
        canvasCustomers = [];
        costs = [];
        var rect = canvasHTML.getBoundingClientRect();
        // push null, it's the depot
        vertex.push([]);
        clicks.innerHTML = "<h2>Customers</h2>";
        i = 1;
        costs = cs.costs;
        for (var c in cs.vertices) {
            if (c == 0) {
                continue;
            }
            var a = [];
            /* calculate real points for canvas */
            if (flagVRP) {
                cs.vertices[c].x = cs.vertices[c].x * cs.size;
                cs.vertices[c].y = cs.vertices[c].y * cs.size;
            }
            a.clientX = cs.vertices[c].x + canvas.width / 2 + rect.left;
            a.clientY = -(cs.vertices[c].y - canvas.height / 2) + rect.top;
            drawCustomer(i, a, false);
            a.clientX -= rect.left;
            a.clientY -= rect.top;
            a.request = cs.vertices[c].request;
            a.time = cs.vertices[c].time;
            if (parseInt(i, 10) < 10)
                a.name = "V0" + i;
            else
			 a.name = "V" + i;
            var id = "<span class='num'>" + "V<sub>" + i + "</sub>" + "</span> ";
            var app = id + $.strPad(cs.vertices[c].x, 4) + "; " +
            $.strPad(cs.vertices[c].y, 4) + "<br/>";
            clicks.innerHTML += app;
            vertex.push(a);
            i++;
        }
        pars[0].value = cs.vehicles;
        pars[1].value = cs.capacity;
        pars[2].value = cs.worktime;
        for (var n in pars)
            pars[n].disabled = true;
        if(Array.isArray(cs.routes))
            drawRoutes(cs.routes);
        var stop = performance.now();
        console.log(stop-start);
        fileupload.value = "";
        flagVRP = false;
    } else {
        alert("Invalid file format!");
        return;
    }
}

/* draw the routes on the canvas */
function drawRoutes(rs) {
    var red =  242;
    var green = 38;
    var blue = 19;
    var path;
    var step;
    var index;
    var r;
    var color;
    routes = [];
    var routeItem;
    for(var h in rs) {
        color = 'rgb(' + red + ', ' + green + ', ' + blue + ')';
        red = (red + 33) % 255;
        green = (green + 47) % 255;
        blue = (blue + 96) % 255;
        /* for each route */
        step = "M " + (centerX + 1) + " " + (centerY - 1) + " L ";
        r = [];
        for (var c in rs[h]) {
            index = rs[h][c].replace(/[^\d\.\-]/g, '');
            if (index !== 0) {
                index = parseInt(index, 10);
                var x = vertex[index].clientX - 2;
                var y = vertex[index].clientY - 3;
                step += x + " " + y + " L ";
                r.push(index);
            }
        }
        step += "z";
        path = new fabric.Path(step);
        path.set({
            fill: '',
            stroke: color,
            strokeWidth: 2,
            selectable: false,
            perPixelTargetFind: true,
            targetFindTolerance: 15,
            uid: "R" + h
        });
        routeItem = {
            index: h,
            text: r,
            path: path,
            draw: true,
            color: color
        };
        routes.push(routeItem);
        canvas.add(path);
    }
    printRoutes();
}

function onMouse(e) {
    var id = -1;
    if (intRegex.test(e.target.uid))
        id = e.target.uid.match(intRegex)[0];
    if (id >= 0) {
        switch (e.target.uid[0]) {
            case "C":
                if (lastCustomer.i === id) {
                    canvas.remove(lastCustomer.text);
                    lastCustomer.i = null;
                    lastCustomer.text = null;
                } else
                    showOneCustomer(id);
                break;
            case "R":
                drawOneRoute(id);
                break;
        }
    }else {

    }
}

/* print the routes in text */
function printRoutes() {
    var contInit = "<span class='num'>" + "V<sub>";
    var contEnd = "</sub>" + "</span> ";
    var arrow = " <span class='arrow'>→</span> ";
    var oSpan;
	var totalCost = 0;
    for (var route in routes) {
        oSpan = document.createElement("div");
        oSpan.setAttribute("id", routes[route].index);
        oSpan.style.borderLeft = "10px solid " + colorToHex(routes[route].color);
        oSpan.style.paddingLeft = "10px";
        oSpan.innerHTML = "<span class='cost'>" + $.strPad(costs[route], 4) + " </span>";
		totalCost += parseInt(costs[route]);
        for (var v in routes[route].text) {
            oSpan.innerHTML += contInit + routes[route].text[v] + contEnd;
            if (v < (routes[route].text.length - 1))
                oSpan.innerHTML += arrow;
        }
        oSpan.onclick = function() {
            drawOneRoute(this.id);
        };
        routeDiv.appendChild(oSpan);
    }
	oSpan = document.createElement("div");
	oSpan.style.color = "#2C3E50";
	oSpan.innerHTML = "Total cost: " + totalCost;
	routeDiv.appendChild(oSpan);
}

function colorToHex(color) {
    if (color.substr(0, 1) === '#') {
        return color;
    }
    var digits = /(.*?)rgb\((\d+), (\d+), (\d+)\)/.exec(color);

    var red = parseInt(digits[2]);
    var green = parseInt(digits[3]);
    var blue = parseInt(digits[4]);

    var rgb = blue | (green << 8) | (red << 16);
    return digits[1] + '#' + rgb.toString(16);
}

var lastRemoved = null;
function drawOneRoute(r) {
    var route = routes[r].path;
    if (lastRemoved === route) {
        drawAll(r);
        lastRemoved = null;
        return;
    }
    if (lastRemoved === null)
        lastRemoved = route;
    else {
        route.draw = true;
        canvas.add(route);
    }
    cleanAll(r);
    lastRemoved = route;
}

var lastCustomer = {
    i: null,
    text: null
};
function showOneCustomer(i) {
    if (lastCustomer.text !== null) {
        canvas.remove(lastCustomer.text);
        lastCustomer.i = null;
        lastCustomer.text = null;
    }
    var customer = vertex[i];
    var req = customer.request;
    var tim = customer.time;
	var name = customer.name;
    var c = new fabric.Text("Name: " + name + "\n" +
							"Req: " + req + "\n" +
                            "Time: "+ tim, {
        fontFamily: 'monospace',
        fontSize: fontSize,
        fontWeight: "bold",
        fill: '#E4F1FE',
        left: customer.clientX - 35,
        top: customer.clientY + 15
    });
    var textContainer = new fabric.Rect({
        left: customer.clientX - 40,
        top: customer.clientY + 10,
        fill: "#D91E18",
        width: 83,
        height: 64,
        rx: 3,
        ry: 3
    });
    var group = new fabric.Group([ textContainer, c ], {
        left: customer.clientX - 40,
        top: customer.clientY + 10
    });
    canvas.add(group);
    lastCustomer.i = i;
    lastCustomer.text = group;
}

function cleanAll(r) {
    for (var route in routes) {
        if (route != r) {
            routes[route].draw = false;
            canvas.remove(routes[route].path);
        }
    }
}

function drawAll(r) {
    for (var route in routes) {
        if (!routes[route].draw && route != r)
            canvas.add(routes[route].path);
    }
}

function convertFile(data) {
	data = data.split("\n");
    var dimension, capacity, iCustomer;
    var i = 0;
    while(data[i].lastIndexOf("EOF") < 0) {
        if (data[i].lastIndexOf("DIMENSION") >= 0) {
            // remove the depot
            dimension = parseInt(data[i].split(" ")[2].trim()) - 1;
        }
        if (data[i].lastIndexOf("CAPACITY") >= 0) {
            capacity = parseInt(data[i].split(" ")[2].trim());
        }
        if (data[i].lastIndexOf("NODE_COORD_SECTION") >= 0 && dimension > 0) {
            i++;
            iCustomer = 0;
            vertex = [];
            var item = {
                "name" : "V0",
                "x": 0,
                "y": 0
            };
            xDepot = parseInt(data[i].split(" ")[1].trim());
            yDepot = parseInt(data[i].split(" ")[2].trim());
            // push depot
            vertex.push(item);
            i++;
            while (iCustomer < dimension) {
                item = "";
                X = parseInt(data[i].split(" ")[1].trim()) - xDepot;
                Y = parseInt(data[i].split(" ")[2].trim()) - yDepot;
                if (iCustomer + 1 < 10)
                    var n = "V0" + (iCustomer + 1);
                else
                    var n = "V" + (iCustomer + 1);
                item = {
                    "name" : n,
                    "x": X,
                    "y": Y,
                    "request": 0,
                    "time" : 0
                };
                vertex.push(item);
                iCustomer++;
                i++;
            }
            i--;
        }
        if (data[i].lastIndexOf("DEMAND_SECTION") >= 0 && dimension > 0 && vertex.length > 1) {
            // jump to the first customer
            i += 2;
            iCustomer = 1;
            // push depot
            while (iCustomer < vertex.length) {
                vertex[iCustomer].request = parseInt(data[i].split(" ")[1].trim());
                iCustomer++;
                i++;
            }
            i--;
        }
        i++;
    }
    if (vertex.length > 1) {
        outputJSON = JSON.stringify({
            "type": "VRP",
            "size": 4,
            vertices: vertex,
            "vehicles": parseInt(pars[0].value, 10),
            "capacity": capacity,
            "worktime": vertex.length,
            "costs": getCostsMatrix(pars[3].value)
        }, null, 4);
        createFile(outputJSON);
    }else
        alert("Something went wrong");
}
