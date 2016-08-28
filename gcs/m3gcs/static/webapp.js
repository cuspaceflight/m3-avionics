var map;
var marker;
function mapInit(){
    map = new google.maps.Map(document.getElementById("map"), {
        center: {lat:55.7163992, lng:-4.8039792},
        zoom: 16,
    });
    
    marker = new google.maps.Marker({
        position: {lat:0, lng:0},
        map: map,
        title: "Rocket"
    });
}

function moveMarker(lat,lng){
    marker.setPosition({lat:lat, lng:lng});
}


// status should contain 2 fields:
//  status: 'ok', 'init' or 'error'
//  reason: if status=='error', why
function setStatus(name, status){
    var statuses = {ok: "panel-success", error: "panel-danger", init: "panel-warning"};
    var namestrs = {ok: ": OK", error: ": " + status.reason, init: ": Init"};
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

function canCommand(parent, name, arg){
    $.post("/command", {parent:parent, name:name, arg:arg});
}

function updateLastHeard(){
    var now = new Date().getTime()/1000;
    for(name in packettypes){
        var lowest = -1;
        for(cidx in packettypes[name]){
            var canid = packettypes[name][cidx];
            if(lastTimes[canid]){
                var secdiff = Math.floor(10*(now - lastTimes[canid]))/10;
                if((lowest == -1) || (secdiff < lowest)){
                    lowest = secdiff;
                }
                secdiff += "s";
            }else{
                var secdiff = "never"
            }
            $("#lasttime-" + canid).text(secdiff);
        }
        if(lowest == -1){
            lowest = "never";
        }else{
            lowest += "s";
        }
        $("#lasttime-" + name).text(lowest);
    }
}

$(document).ready(function(){
    setInterval(function(){
        $.ajax({
            dataType: "json",
            url: "/state",
            success: function(js){
                var toflash = new Array();
                for(idx in js['lasttimes']){
                    for(can_id in js['lasttimes'][idx]){
                        if(lastTimes[can_id] < js['lasttimes'][idx][can_id]){
                            // blink the datapoints which updated this time
                            toflash.push("#lasttime-" + can_id);
                        }
                        lastTimes[can_id] = js['lasttimes'][idx][can_id];
                    }
                }
                var state = js['state'];
                for(idx in state){
                    var names = Object.keys(state[idx]);
                    names.sort();
                    var sorted = [];
                    for(id in names){
                        var name = names[id]
                        if(name == "Status"){
                            var reasonidx = state[idx][name].indexOf(":");
                            var reason = "not implemented";
                            var status = "";
                            if(reasonidx == -1){
                                status = state[idx][name];
                            }else{
                                status = state[idx][name].substring(0,reasonidx);
                                reason = state[idx][name].substring(reasonidx+2);
                            }
                            setStatus(idx, {status: status.toLowerCase().trim(), reason: reason});
                        }
                        sorted.push("<tr><td>" + name + "</td>" +
                            "<td>" + state[idx][name].replace(/\n/g, "<br />") + "</td>" +
                            "<td id='lasttime-" + packettypes[idx][name] + "'></td>" +
                            "</td></tr>");
                    }
                    $("#display-" + idx).html(
                        "<table class='table table-condensed'>" +
                        sorted.join("\n") +
                        "</table>");
                    if(idx == "m3radio"){
                        try{
                            var latlng = state[idx]['GPS Lat/Long'].split(" ");
                            moveMarker(parseFloat(latlng[0]), parseFloat(latlng[1]));
                        }catch{
                            
                        }
                    }
                }

                for(idx in toflash){
                    var el = $(toflash[idx]);
                    el.fadeOut().fadeIn();
                }

                updateLastHeard();

            }
        });
    }, 200);
});
