// status should contain 2 fields:
//  status: 'ok', 'init' or 'error'
//  reason: if status=='error', why
function setStatus(name, status){
    var statuses = {ok: "panel-success", error: "panel-danger", init: "panel-warning"};
    var namestrs = {ok: ": OK", error: ": " + status.reason, init: ": Init"};
    $("#header-" + name).removeClass("panel-default panel-danger panel-warning panel-success").addClass(statuses[status.status]);
    $("#header-" + name + " h3").text(name + namestrs[status.status]);
}

var packettypes = new Array();
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

function canCommand(parent, name, arg){
    $.post("/command", {parent:parent, name:name, arg:arg});
}

$(document).ready(function(){
    setInterval(function(){
        var now = new Date().getTime();
        for(idx in packettypes){
            var name = packettypes[idx][0];
            var canid = packettypes[idx][1];
            if(lastTimes[canid]){
                var secdiff = ((now - lastTimes[canid])/1000) + "s ago";
            }else{
                var secdiff = "never"
            }
            $("#lasttime-" + canid).text(secdiff);
            $("#lasttime-" + name).text(secdiff);
        }
    }, 1000);
    
    setInterval(function(){
        $.getJSON("/state", function(js){
            for(idx in js){
                var names = Object.keys(js[idx]);
                names.sort();
                var sorted = [];
                for(id in names){
                    var name = names[id]
                    if(name == "Status"){
                        var reasonidx = js[idx][name].indexOf(":");
                        var reason = "not implemented";
                        var status = "";
                        if(reasonidx == -1){
                            status = js[idx][name];
                        }else{
                            status = js[idx][name].substring(0,reasonidx);
                            reason = js[idx][name].substring(reasonidx+2);
                        }
                        setStatus(idx, {status: status.toLowerCase().trim(), reason: reason});
                    }
                    sorted.push("<tr><td>" + name + "</td><td>" + js[idx][name].replace(/\n/g, "<br />") + "</tr>");
                }
                $("#display-" + idx).html(
                    "<table class='table table-condensed'>" +
                    sorted.join("\n") +
                    "</table>");
            }
        });
    }, 100);
});
