// status should contain 2 fields:
//  status: 'good', 'init' or 'error'
//  reason: if status=='error', why
function setStatus(name, status){
    var statuses = {good: "panel-success", error: "panel-danger", init: "panel-warning"};
    var namestrs = {good: ": OK", error: ": " + status.reason, init: ": Init"};
    $("#header-" + name).removeClass("panel-default panel-danger panel-warning panel-success").addClass(statuses[status.status]);
    $("#header-" + name + " h3").text(name + namestrs[status.status]);
}

var packettypes = {};
var lastTimes = {};

// state should contain at least 2 field:
//  msgId: 
//  logstring: friendly string to print in the packet log
// other fields may be present for custom disaply on the UI
function stateUpdate(name, state){
    var el = $("#log-window");
    el.append(state.logstring + "\n");
    el.scrollTop(el[0].scrollHeight); // Scroll to the bottom
    lastTimes[state.id] = new Date().getTime(); // Update the last packet time
}

$(document).ready(function(){
    setInterval(function(){
        var now = new Date().getTime();
        for(idx in names){
            var name = names[idx];
            var secdiff = (now - lastTimes[name])/1000;
            $("#lasttime-" + name).text(secdiff + "s");
        }
    }, 1000);
});
