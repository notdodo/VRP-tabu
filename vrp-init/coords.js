var i = 1;
var canvasHTML = document.getElementById("coords"),
    canvas = new fabric.Canvas("coords");
var rect = canvasHTML.getBoundingClientRect();
var clicks = document.getElementById("clicks");
var currentPos = document.getElementById("coord_msg");
var fileupload = document.getElementById("fileupload");
var loadedPrinted = false;
var loadedFile = null;
var loadedFileModified = 0;
var autoReloadFileName = "E-n51-k5.json";
var routeDiv = document.getElementById("routes");
var instanceStatus = document.getElementById("instance_status");
var loadStatus = document.getElementById("load_status");
var routeSummary = document.getElementById("route_summary");
var themeToggle = document.getElementById("theme_toggle");
var reloadIntervalInput = document.getElementById("reload_interval");
var pars = $(".parameter-grid input[type='number']");
// vertex and routes array
var vertex = [],
    routes = [],
    canvasCustomers = [],
    costs = [];
var vertexByName = Object.create(null);
var centerX = canvas.width / 2 - 6,
    centerY = canvas.height / 2 - 2;
var fontSize = 14;
var intRegex = /(\d+)/;
var flagVRP = false;
var fileName = "";
var textFile = null;
var editingEnabled = true;
var movingCustomer = false;
var reloadTimer = null;

/* display the center of the canvas */
$(function() {
    var play = document.getElementById("play");
    play.checked = false;
    play.value = 0;
    if (themeToggle) {
        themeToggle.checked = document.body.classList.contains("theme-dark");
        setTheme(themeToggle.checked);
    }
    resetCustomersList();
    resetRouteView("No routes");

    // Depot
    canvas.add(
        new fabric.Text("■", {
            fontFamily: "Roboto",
            fontSize: fontSize,
            fill: "white",
            left: centerX - 5,
            top: centerY - 7,
            selectable: false
        })
    );
    canvas.add(
        new fabric.Text("(0;0)", {
            fontFamily: "sans-serif",
            fontSize: fontSize + 2,
            fill: "white",
            left: centerX - 18,
            top: centerY + 8,
            selectable: false,
            uid: "depot"
        })
    );

    vertex.push(makeDepotVertex());
    setEditing(true);
    setStatus("Ready");

    // open a file
    fileupload.addEventListener("change", readSingleFile, false);
    // on mouse move event
    canvas.on(
        "mouse:move",
        evt => {
            var mousePos = getMousePos(evt.e);
            var message = parseInt(mousePos.x, 10) + ", " + parseInt(mousePos.y, 10);
            currentPos.innerHTML = message;
        },
        false
    );

    canvas.on(
        "object:moving",
        options => {
            movingCustomer = true;
            var match = options.target.uid && options.target.uid.match(intRegex);
            if (!match) return;
            var idCust = parseInt(match[0], 10);
            var mousePos = getMousePos(options.e);
            if (vertex[idCust]) {
                vertex[idCust].x = mousePos.x;
                vertex[idCust].y = mousePos.y;
                renderAllCustomers();
            }
        },
        false
    );

    canvas.on(
        "mouse:down",
        () => {
            movingCustomer = false;
        },
        false
    );

    canvas.on(
        "mouse:up",
        evt => {
            if (!editingEnabled || movingCustomer || evt.target) {
                movingCustomer = false;
                return;
            }
            addCustomerFromEvent(evt.e);
            movingCustomer = false;
        },
        false
    );
});

function pluralize(count, label) {
    return count + " " + label + (count === 1 ? "" : "s");
}

function setStatus(message, state) {
    if (!loadStatus) return;
    loadStatus.textContent = message;
    loadStatus.className = state || "";
}

function setTheme(enabled) {
    if (!document.body) return;
    document.body.classList.toggle("theme-dark", enabled);
    document.body.classList.toggle("theme-light", !enabled);
    if (themeToggle) themeToggle.checked = enabled;
}

function setEditing(enabled) {
    editingEnabled = enabled;
    pars.prop("disabled", !enabled);
    updateInstanceStatus();
}

function updateInstanceStatus() {
    if (!instanceStatus) return;
    var customers = Math.max(vertex.length - 1, 0);
    var routeText = routes.length ? " · " + pluralize(routes.length, "route") : "";
    var mode = editingEnabled ? "editable" : "loaded";
    instanceStatus.textContent = pluralize(customers, "customer") + routeText + " · " + mode;
    updateCustomerCount();
}

function updateCustomerCount() {
    var customerCount = document.getElementById("customer_count");
    if (customerCount) {
        customerCount.textContent = pluralize(Math.max(vertex.length - 1, 0), "customer");
    }
}

function resetCustomersList() {
    clicks.innerHTML =
        "<div class='section-title'><h2>Customers</h2><span id='customer_count'>0 customers</span></div>" +
        "<div id='customer_list' class='customer-list'><div class='empty-state'>No customers</div></div>";
}

function renderAllCustomers() {
    resetCustomersList();
    for (var c = 1; c < vertex.length; c++) {
        appendCustomerRow(c, vertex[c].x, vertex[c].y, vertex[c].request, vertex[c].time);
    }
    updateInstanceStatus();
}

function appendCustomerRow(index, x, y, request, time) {
    var list = document.getElementById("customer_list");
    if (!list) return;
    var empty = list.querySelector(".empty-state");
    if (empty) list.removeChild(empty);

    var row = document.createElement("div");
    row.className = "customer-row";

    var name = document.createElement("span");
    name.className = "num";
    name.innerHTML = "V<sub>" + index + "</sub>";

    var detail = document.createElement("span");
    detail.className = "customer-detail";
    detail.textContent = formatNumber(x) + "; " + formatNumber(y);

    if (hasValue(request) || hasValue(time)) {
        var extra = document.createElement("span");
        extra.className = "customer-extra";
        extra.textContent =
            "  q " + formatOptionalNumber(request) + " / t " + formatOptionalNumber(time);
        detail.appendChild(extra);
    }

    row.appendChild(name);
    row.appendChild(detail);
    list.appendChild(row);
    updateCustomerCount();
}

function resetRouteView(message) {
    routeDiv.innerHTML = "";
    var empty = document.createElement("div");
    empty.className = "empty-state";
    empty.textContent = message || "No routes";
    routeDiv.appendChild(empty);
    updateRouteSummary(0, null);
    lastRemoved = null;
}

function updateRouteSummary(count, totalCost) {
    if (!routeSummary) return;
    routeSummary.textContent = pluralize(count, "route");
    if (hasValue(totalCost)) routeSummary.textContent += " · total " + totalCost;
    updateInstanceStatus();
}

function makeDepotVertex() {
    return {
        name: "V0",
        x: 0,
        y: 0,
        clientX: centerX + 1,
        clientY: centerY - 1,
        request: 0,
        time: 0
    };
}

function rebuildVertexNameIndex() {
    vertexByName = Object.create(null);
    for (var index = 0; index < vertex.length; index++) {
        if (vertex[index] && vertex[index].name) {
            vertexByName[String(vertex[index].name)] = index;
        }
    }
}

function formatVertexName(index) {
    return index < 10 ? "V0" + index : "V" + index;
}

function hasValue(value) {
    return value !== null && typeof value !== "undefined" && value !== "";
}

function isNumericValue(value) {
    return hasValue(value) && !Array.isArray(value) && Number.isFinite(Number(value));
}

function formatOptionalNumber(value) {
    return isNumericValue(value) ? formatNumber(value) : "-";
}

function formatNumber(value) {
    var number = Number(value);
    if (Number.isFinite(number)) return Math.round(number);
    return value;
}

function refreshCanvasRect() {
    rect = canvasHTML.getBoundingClientRect();
}

function resetRouteHoverHandlers() {
    if (typeof canvas.off === "function") {
        canvas.off("mouse:over", onMouse);
        canvas.off("mouse:out", onMouse);
    } else if (canvas.__eventListeners) {
        canvas.__eventListeners["mouse:over"] = [];
        canvas.__eventListeners["mouse:out"] = [];
    }
    canvas.on("mouse:over", onMouse);
    canvas.on("mouse:out", onMouse);
}

function addCustomerFromEvent(evt) {
    var mousePos = getMousePos(evt);
    var x = mousePos.x,
        y = mousePos.y;
    drawCustomer(i, evt);
    var item = {
        name: formatVertexName(i),
        x: x,
        y: y
    };
    vertex.push(item);
    rebuildVertexNameIndex();
    appendCustomerRow(i, x, y);
    i++;
    updateInstanceStatus();
    setStatus("Customer added");
}

function playStop(c) {
    if (c.checked) {
        if (window.location.protocol === "file:") {
            c.checked = false;
            c.value = 0;
            loadedPrinted = false;
            setStatus("Serve with make serve-view", "error");
            return;
        }
        if (!fileName) fileName = autoReloadFileName;
        loadedPrinted = true;
        c.value = 1;
        startReloadTimer();
        setStatus("Auto reload every " + getReloadDelaySeconds() + "s: " + fileName);
        loadJSON();
    } else {
        loadedPrinted = false;
        c.value = 0;
        stopReloadTimer();
        setStatus("Auto reload off");
    }
}

function getReloadDelaySeconds() {
    var seconds = parseInt(reloadIntervalInput && reloadIntervalInput.value, 10);
    if (!Number.isFinite(seconds) || seconds < 1) seconds = 4;
    if (seconds > 60) seconds = 60;
    if (reloadIntervalInput) reloadIntervalInput.value = seconds;
    return seconds;
}

function startReloadTimer() {
    stopReloadTimer();
    reloadTimer = window.setInterval(runWorker, getReloadDelaySeconds() * 1000);
}

function stopReloadTimer() {
    if (reloadTimer !== null) {
        window.clearInterval(reloadTimer);
        reloadTimer = null;
    }
}

function updateReloadInterval() {
    var seconds = getReloadDelaySeconds();
    if (loadedPrinted) {
        startReloadTimer();
        setStatus("Auto reload every " + seconds + "s: " + fileName);
    }
}

function loadJSON() {
    if (!loadedPrinted) return;
    if (window.location.protocol !== "file:") {
        if (!fileName) fileName = autoReloadFileName;
        fetchReloadedFile();
        return;
    }
    if (loadedFile) {
        reloadSelectedFile();
        return;
    }
    if (!fileName || window.location.protocol === "file:") {
        setStatus("Auto reload needs selected file", "error");
        return;
    }
    var xobj = new XMLHttpRequest();
    xobj.overrideMimeType("application/json");
    xobj.open("GET", fileName, true);
    xobj.onreadystatechange = function() {
        if (xobj.readyState == 4 && (xobj.status == "200" || xobj.status === 0)) {
            try {
                parseCustomer(JSON.parse(xobj.responseText));
            } catch (error) {
                setStatus("Auto reload failed", "error");
            }
        }
    };
    xobj.send(null);
}

function fetchReloadedFile() {
    fetch(fileName + "?_=" + Date.now(), {
        cache: "no-store"
    })
        .then(function(response) {
            if (!response.ok) {
                throw new Error("HTTP " + response.status);
            }
            return response.json();
        })
        .then(function(data) {
            parseCustomer(data);
            setStatus("Auto reloaded " + fileName);
        })
        .catch(function() {
            if (loadedFile) {
                reloadSelectedFile();
            } else {
                setStatus("Auto reload failed", "error");
            }
        });
}

function reloadSelectedFile() {
    if (!loadedFile) return;
    if (loadedFile.lastModified === loadedFileModified) {
        return;
    }
    var reader = new FileReader();
    reader.onload = function(event) {
        try {
            parseCustomer(JSON.parse(event.target.result));
            loadedFileModified = loadedFile.lastModified;
            setStatus("Auto reloaded " + loadedFile.name);
        } catch (error) {
            setStatus("Auto reload failed", "error");
        }
    };
    reader.onerror = function() {
        setStatus("Auto reload failed", "error");
    };
    reader.readAsText(loadedFile);
}

var blob = new Blob(
    ["self.onmessage = function(event) { postMessage(event.data); }"], {
        type: "application/javascript"
    }
);
var worker = new Worker(URL.createObjectURL(blob));
worker.onmessage = function(event) {
    loadJSON();
};

function runWorker() {
    worker.postMessage("run");
}

/* get the mouse position on the canvas */
function getMousePos(evt) {
    refreshCanvasRect();
    return {
        x: evt.clientX - rect.left - canvasHTML.width / 2,
        y: -(evt.clientY - rect.top - canvasHTML.height / 2)
    };
}

/* onclick draw a customer */
function drawCustomer(uid, evt, drag) {
    if (typeof drag === "undefined") drag = true;
    refreshCanvasRect();
    var x = evt.clientX - rect.left,
        y = evt.clientY - rect.top;
    var last = new fabric.Text("⌂", {
        fontFamily: "Roboto",
        fontSize: fontSize,
        fill: "#dfe6e9",
        left: x - 6,
        top: y - 10,
        hasBorders: false,
        hasControls: false,
        hasRotatingPoint: false,
        selectable: drag,
        targetFindTolerance: 2,
        uid: "C" + uid
    });
    canvas.add(last);
    canvasCustomers[uid] = last;
}
//pads left
$.strPad = (s, l) => {
    var o = s.toString();
    var dif = l - o.length;
    if (i <= 9) dif++;
    o = Array(dif + 1).join("&nbsp;") + s;
    return o;
};

/* get a random int from min to max */
function getRandomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

/* add request and service time per each customer */
function setRequestTime(par) {
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
        for (var j = 0; j < vertex.length; j++) {
            if (k !== j) {
                var item = {
                    from: k,
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
    renderAllCustomers();
    if (vertex.length > 1) {
        var outputJSON = JSON.stringify({
                vertices: vertex,
                vehicles: parseInt(pars[0].value, 10),
                capacity: parseInt(pars[1].value, 10),
                worktime: parseInt(pars[2].value, 10),
                costs: getCostsMatrix(pars[3].value)
            },
            null,
            4
        );
        createFile(outputJSON);
        setStatus("Generated input.json");
    } else {
        setStatus("No customers to export", "error");
        alert("Please insert some customers");
    }
}

function createFile(data) {
    var toSave = new Blob([data], {
        type: "text/plain"
    });
    if (textFile !== null) {
        window.URL.revokeObjectURL(textFile);
    }
    textFile = window.URL.createObjectURL(toSave);
    var link = document.getElementById("download");
    link.href = textFile;
    link.click();
}

function readSingleFile(evt) {
    resetRouteView("No routes");
    //Retrieve the first (and only!) File from the FileList object
    var f = evt.target.files[0];

    if (f) {
        loadedFile = f;
        loadedFileModified = f.lastModified;
        var r = new FileReader();
        r.onload = function(e) {
            var contents = e.target.result;
            fileName = f.name;
            var extension = f.name.substr((~-f.name.lastIndexOf(".") >>> 0) + 2).toLowerCase();
            try {
                if (extension === "json") {
                    var data = JSON.parse(contents);
                    parseCustomer(data);
                } else if (extension === "vrp") {
                    convertFile(contents);
                } else {
                    setStatus("Unsupported file type", "error");
                    alert("Invalid file format!");
                }
            } catch (error) {
                setStatus("Failed to load file", "error");
                alert("Failed to load file: " + error.message);
            }
        };
        r.onerror = function(e) {
            setStatus("Failed to read file", "error");
            alert("Errore!");
        };
        r.readAsText(f);
    } else {
        setStatus("Failed to load file", "error");
        alert("Failed to load file");
    }
}

/* parse the json and display the results */
/* ~200 microseconds */
function parseCustomer(cs) {
    resetRouteView("No routes");
    flagVRP = cs.type === "VRP";
    if (Array.isArray(cs.vertices)) {
        // before draw clean all the canvas
        resetRouteHoverHandlers();
        resetCustomersList();
        for (var e in canvasCustomers) canvas.remove(canvasCustomers[e]);
        for (var r in routes) canvas.remove(routes[r].path);
        vertex = [];
        routes = [];
        canvasCustomers = [];
        costs = [];
        refreshCanvasRect();
        vertex.push(makeDepotVertex());
        i = 1;
        costs = Array.isArray(cs.route_costs) ? cs.route_costs : cs.costs;
        for (var c in cs.vertices) {
            if (c == 0) {
                continue;
            }
            var source = cs.vertices[c];
            var displayX = source.x;
            var displayY = source.y;
            /* calculate real points for canvas */
            if (flagVRP) {
                displayX = source.x * (cs.size + 1);
                displayY = source.y * (cs.size + 1);
            }
            var a = {
                clientX: displayX + canvas.width / 2 + rect.left,
                clientY: -(displayY - canvas.height / 2) + rect.top
            };
            drawCustomer(i, a, false);
            a.clientX -= rect.left;
            a.clientY -= rect.top;
            a.x = displayX;
            a.y = displayY;
            a.request = source.request;
            a.time = source.time;
            a.name = source.name || formatVertexName(i);
            vertex.push(a);
            appendCustomerRow(i, displayX, displayY, source.request, source.time);
            i++;
        }
        rebuildVertexNameIndex();
        pars[0].value = cs.vehicles;
        pars[1].value = cs.capacity;
        pars[2].value = cs.worktime;
        setEditing(false);
        if (Array.isArray(cs.routes)) drawRoutes(cs.routes);
        fileupload.value = "";
        flagVRP = false;
        setStatus("Loaded " + pluralize(vertex.length - 1, "customer"));
        updateInstanceStatus();
    } else {
        setStatus("Invalid file format", "error");
        alert("Invalid file format!");
        return;
    }
}

/* draw the routes on the canvas */
function drawRoutes(rs) {
    rebuildVertexNameIndex();
    var routeColors = [
        "#d7263d",
        "#1b998b",
        "#2e86ab",
        "#f46036",
        "#6f2dbd",
        "#4d908e",
        "#f9a620",
        "#2d6cdf"
    ];
    var path;
    var step;
    var index;
    var r;
    var color;
    for (var oldRoute in routes) canvas.remove(routes[oldRoute].path);
    routes = [];
    var routeItem;
    for (var h in rs) {
        if (!Array.isArray(rs[h])) continue;
        color = routeColors[routes.length % routeColors.length];
        /* for each route */
        step = "M " + (centerX + 1) + " " + (centerY - 1);
        r = [];
        for (var c in rs[h]) {
            index = getRouteIndex(rs[h][c]);
            if (index > 0 && vertex[index]) {
                var x = vertex[index].clientX - 2;
                var y = vertex[index].clientY - 3;
                step += " L " + x + " " + y;
                r.push(index);
            }
        }
        if (r.length === 0) continue;
        step += " Z";
        path = new fabric.Path(step);
        path.set({
            fill: "",
            stroke: color,
            strokeWidth: 2.5,
            selectable: false,
            perPixelTargetFind: true,
            targetFindTolerance: 15,
            uid: "R" + routes.length
        });
        routeItem = {
            index: routes.length,
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

function getRouteIndex(value) {
    if (typeof value === "number") return value;
    var label = String(value);
    if (Object.prototype.hasOwnProperty.call(vertexByName, label)) {
        return vertexByName[label];
    }
    var numeric = label.match(/^\s*(-?\d+)\s*$/);
    if (numeric) return parseInt(numeric[1], 10);
    var generatedName = label.match(/^V0*(\d+)$/i);
    if (generatedName) return parseInt(generatedName[1], 10);
    return -1;
}

function onMouse(e) {
    if (!e.target || !e.target.uid) return;
    var match = e.target.uid.match(intRegex);
    if (!match) return;
    var id = match[0];
    switch (e.target.uid[0]) {
        case "C":
            if (lastCustomer.i === id) {
                canvas.remove(lastCustomer.text);
                lastCustomer.i = null;
                lastCustomer.text = null;
            } else showOneCustomer(id);
            break;
        case "R":
            drawOneRoute(id);
            break;
    }
}

/* print the routes in text */
function printRoutes() {
    if (!routes.length) {
        resetRouteView("No routes");
        return;
    }

    routeDiv.innerHTML = "";
    var totalCost = 0;
    var hasCosts = false;
    for (var route in routes) {
        var routeItem = routes[route];
        var routeCost = getRouteCost(routeItem.text, parseInt(route, 10));
        var routeLoad = getRouteLoad(routeItem.text);
        var oSpan = document.createElement("div");
        oSpan.className = "route-row";
        oSpan.setAttribute("data-route-index", routeItem.index);
        oSpan.style.borderLeftColor = routeItem.color;

        var pathHtml = routeVertexHtml(0);
        for (var v in routeItem.text) {
            pathHtml += " <span class='arrow'>→</span> " + routeVertexHtml(routeItem.text[v]);
        }
        pathHtml += " <span class='arrow'>→</span> " + routeVertexHtml(0);

        var meta = pluralize(routeItem.text.length, "stop");
        if (hasValue(routeLoad)) meta += " · load " + routeLoad;
        if (hasValue(routeCost)) {
            totalCost += routeCost;
            hasCosts = true;
        }

        oSpan.innerHTML =
            "<span class='route-cost'>" +
            (hasValue(routeCost) ? "cost " + routeCost : "cost -") +
            "</span><span class='route-path'>" +
            pathHtml +
            "</span><span class='route-meta'>" +
            meta +
            "</span>";
        oSpan.onclick = function() {
            drawOneRoute(this.getAttribute("data-route-index"));
        };
        routeDiv.appendChild(oSpan);
    }

    var total = document.createElement("div");
    total.className = "route-total";
    total.textContent = hasCosts ? "Total cost: " + totalCost : "Route costs unavailable";
    routeDiv.appendChild(total);
    updateRouteSummary(routes.length, hasCosts ? totalCost : null);
}

function routeVertexHtml(index) {
    return "<span class='num'>V<sub>" + index + "</sub></span>";
}

function getRouteLoad(routeIndexes) {
    var total = 0;
    var hasDemand = false;
    for (var routeIndex in routeIndexes) {
        var customer = vertex[routeIndexes[routeIndex]];
        if (customer && isNumericValue(customer.request)) {
            total += Number(customer.request);
            hasDemand = true;
        }
    }
    return hasDemand ? total : null;
}

function getRouteCost(routeIndexes, routePosition) {
    if (Array.isArray(costs)) {
        var listedCost = costs[routePosition];
        if (isNumericValue(listedCost)) return Math.round(Number(listedCost));
    }
    return calculateRouteCost(routeIndexes);
}

function calculateRouteCost(routeIndexes) {
    var sequence = [0].concat(routeIndexes).concat([0]);
    var total = 0;
    for (var s = 0; s < sequence.length - 1; s++) {
        var arcCost = getArcCost(sequence[s], sequence[s + 1]);
        if (!hasValue(arcCost)) return null;
        total += Number(arcCost);
    }
    return Math.round(total);
}

function getArcCost(from, to) {
    if (!Array.isArray(costs) || !Array.isArray(costs[from])) return null;
    var row = costs[from];
    for (var c = 0; c < row.length; c++) {
        var arc = row[c];
        if (arc && typeof arc === "object" && parseInt(arc.to, 10) === to) {
            return arc.value;
        }
        if (c === to && isNumericValue(arc)) {
            return arc;
        }
    }
    return null;
}

var lastRemoved = null;

function drawOneRoute(r) {
    r = parseInt(r, 10);
    if (!routes[r]) return;
    var route = routes[r].path;
    if (lastRemoved === route) {
        drawAll(r);
        lastRemoved = null;
        markActiveRoute(null);
        setStatus("Showing all routes");
        return;
    }
    if (lastRemoved !== null) {
        routes[r].draw = true;
        canvas.add(route);
    }
    cleanAll(r);
    routes[r].draw = true;
    lastRemoved = route;
    markActiveRoute(r);
    setStatus("Focused route " + (r + 1));
}

function markActiveRoute(r) {
    var rows = routeDiv.querySelectorAll(".route-row");
    for (var row = 0; row < rows.length; row++) {
        rows[row].classList.toggle(
            "is-active",
            r !== null && rows[row].getAttribute("data-route-index") == r
        );
    }
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
    var req = formatOptionalNumber(customer.request);
    var tim = formatOptionalNumber(customer.time);
    var name = customer.name;
    var c = new fabric.Text(
        "Name: " + name + "\n" + "Req: " + req + "\n" + "Time: " + tim, {
            fontFamily: "monospace",
            fontSize: fontSize,
            fontWeight: "bold",
            fill: "#E4F1FE",
            left: customer.clientX - 35,
            top: customer.clientY + 15
        }
    );
    var textContainer = new fabric.Rect({
        left: customer.clientX - 40,
        top: customer.clientY + 10,
        fill: "#D91E18",
        width: 83,
        height: 64,
        rx: 3,
        ry: 3
    });
    var group = new fabric.Group([textContainer, c], {
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
        if (!routes[route].draw && route != r) {
            routes[route].draw = true;
            canvas.add(routes[route].path);
        }
    }
    if (routes[r]) routes[r].draw = true;
}

function convertFile(data) {
    data = data.split("\n");
    var dimension, capacity, iCustomer;
    var i = 0;
    while (data[i].lastIndexOf("EOF") < 0) {
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
                name: "V0",
                x: 0,
                y: 0
            };
            var xDepot = parseInt(data[i].split(" ")[1].trim(), 10);
            var yDepot = parseInt(data[i].split(" ")[2].trim(), 10);
            // push depot
            vertex.push(item);
            i++;
            while (iCustomer < dimension) {
                var X = parseInt(data[i].split(" ")[1].trim(), 10) - xDepot;
                var Y = parseInt(data[i].split(" ")[2].trim(), 10) - yDepot;
                var n = iCustomer + 1 < 10 ? "V0" + (iCustomer + 1) : "V" + (iCustomer + 1);
                item = {
                    name: n,
                    x: X,
                    y: Y,
                    request: 0,
                    time: 0
                };
                vertex.push(item);
                iCustomer++;
                i++;
            }
            i--;
        }
        if (
            data[i].lastIndexOf("DEMAND_SECTION") >= 0 &&
            dimension > 0 &&
            vertex.length > 1
        ) {
            // jump to the first customer
            i += 2;
            iCustomer = 1;
            // push depot
            while (iCustomer < vertex.length) {
                vertex[iCustomer].request = parseInt(data[i].split(" ")[1].trim(), 10);
                iCustomer++;
                i++;
            }
            i--;
        }
        i++;
    }
    if (vertex.length > 1) {
        rebuildVertexNameIndex();
        var outputJSON = JSON.stringify({
                type: "VRP",
                size: 4,
                vertices: vertex,
                vehicles: parseInt(pars[0].value, 10),
                capacity: capacity,
                worktime: vertex.length,
                costs: getCostsMatrix(pars[3].value)
            },
            null,
            4
        );
        createFile(outputJSON);
        setStatus("Converted VRP to JSON");
    } else {
        setStatus("VRP conversion failed", "error");
        alert("Something went wrong");
    }
}
