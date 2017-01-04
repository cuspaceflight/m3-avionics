var map;
var marker;
var line;
var path;
function mapInit(){
    map = new google.maps.Map(document.getElementById("map"), {
        center: {lat:52, lng:0},
        zoom: 16,
    });
    
    marker = new google.maps.Marker({
        position: {lat:0, lng:0},
        map: map,
        title: "Rocket"
    });

    path = new google.maps.MVCArray();
    line = new google.maps.Polyline({
        map: map,
        strokeColor: "#f00",
        strokeWeight: 2,
        strokeOpacity: 1,
        path: path
    });
}

function moveMarker(lat,lng){
    var pos = new google.maps.LatLng({lat:lat, lng:lng});
    marker.setPosition(pos);
    path.push(pos);
}

function canCommand(parent, name, arg){
    $.post("/command", {parent:parent, name:name, arg:arg});
}

var gcs = new GCS();
