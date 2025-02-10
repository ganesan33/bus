// Define variables in the global scope
let map;
let routeLayers = []; // Array to store route layers (max 2)
let routeMarkers = []; // Array to store markers for each route (max 2)
const defaultLocation = [13.0827, 80.2707];
const defaultZoom = 12;
const MAX_ROUTES = 2;

document.addEventListener("DOMContentLoaded", function () {
    initializeMap();

    document.getElementById("search-btn").addEventListener("click", function () {
        const busNumber = document.getElementById("bus-number").value.trim();

        if (!busNumber) {
            alert("Please enter a bus number.");
            return;
        }

        fetch(`/get/${busNumber}`)
            .then(response => response.json())
            .then(data => {
                if (data.error) {
                    alert("Bus not found!");
                    return;
                }

                document.getElementById("bb").innerHTML = `Bus Details:`;
                // Update bus details section
                document.getElementById("bus-number-display").textContent = `Bus Number: ${data.busNumber}` || "N/A";
                document.getElementById("bus-name").textContent = `Bus Name: ${data.busName}` || "N/A";
                document.getElementById("bus-availability").textContent = `Available Seats: ${data.availability}` || "N/A";
             
                // Update list of stops
                document.getElementById("a").innerHTML = `Route Stops:`;
                let routesList = document.getElementById("bus-routes");
                routesList.innerHTML = "";
                data.Routes.forEach(route => {
                    let listItem = document.createElement("li");
                    listItem.textContent = route;
                    routesList.appendChild(listItem);
                });

                if (`${data.availability}` === "0") {
                    document.getElementById("b").innerHTML = `Nearby Buses (within 5 km)`;
                    let nearbyBusesList = document.getElementById("nearby-buses");
                    nearbyBusesList.innerHTML = "";
                    if (data.nearby_buses.length > 1) {
                        data.nearby_buses.forEach(bus => {
                            let listItem = document.createElement("li");
                            if (bus.busNumber !== `${data.busNumber}`) {
                                listItem.textContent = `Bus ${bus.busNumber} - ${bus.busName} (${bus.distance} km)`;
                                nearbyBusesList.appendChild(listItem);
                            }
                        });
                    } else {
                        nearbyBusesList.innerHTML = "No nearby buses found within 5 km";
                    }
                } else {
                    document.getElementById("b").innerHTML = "";
                    document.getElementById("nearby-buses").innerHTML = "";
                }

                // Add new route to map
                displayRoute(data.src, data.dest, data.busNumber);
            })
            .catch(error => console.error("Error fetching bus data:", error));
    });
});

function initializeMap() {
    // If map already exists, remove it
    if (map) {
        map.remove();
    }

    // Create new map instance
    map = L.map("map").setView(defaultLocation, defaultZoom);
    L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
        attribution: "© OpenStreetMap contributors"
    }).addTo(map);
}

function displayRoute(source, destination, busNumber) {
    // If we already have maximum routes, remove the oldest one
    if (routeLayers.length >= MAX_ROUTES) {
        // Remove oldest route and its markers
        const oldestRoute = routeLayers.shift();
        const oldestMarkers = routeMarkers.shift();
        
        // Remove from map
        map.removeLayer(oldestRoute);
        oldestMarkers.forEach(marker => map.removeLayer(marker));
    }

    // Create markers for the new route
    const currentMarkers = [];
    const sourceMarker = L.marker(source)
        .addTo(map)
        .bindPopup(`Bus ${busNumber} - Source`)
        .openPopup();
    const destMarker = L.marker(destination)
        .addTo(map)
        .bindPopup(`Bus ${busNumber} - Destination`);
    
    currentMarkers.push(sourceMarker, destMarker);
    routeMarkers.push(currentMarkers);

    // Get route and display
    const osrmUrl = `https://router.project-osrm.org/route/v1/driving/${source[1]},${source[0]};${destination[1]},${destination[0]}?overview=full&geometries=geojson`;

    fetch(osrmUrl)
        .then(response => response.json())
        .then(data => {
            const routeCoords = data.routes[0].geometry.coordinates.map(coord => [coord[1], coord[0]]);
            
            // Create new route with a unique color based on position
            const routeColor = routeLayers.length === 0 ? "blue" : "red";
            const newRoute = L.polyline(routeCoords, { 
                color: routeColor, 
                weight: 5,
                opacity: 0.8
            }).addTo(map);

            // Add route to our collection
            routeLayers.push(newRoute);

            // Fit map bounds to show all routes
            const allRoutes = L.featureGroup(routeLayers);
            map.fitBounds(allRoutes.getBounds(), { padding: [50, 50] });
        })
        .catch(error => console.error("Error fetching route:", error));
}

// Global function to remove markers that can be called directly from onclick
function removeMarkers() {
    // Remove all routes
    routeLayers.forEach(route => map.removeLayer(route));
    routeLayers = [];

    // Remove all markers
    routeMarkers.forEach(markerSet => {
        markerSet.forEach(marker => map.removeLayer(marker));
    });
    routeMarkers = [];

    // Clear bus details
    document.getElementById("bb").innerHTML = "";
    document.getElementById("bus-number-display").textContent = "";
    document.getElementById("bus-name").textContent = "";
    document.getElementById("bus-availability").textContent = "";

    // Clear routes list
    document.getElementById("a").innerHTML = "";
    document.getElementById("bus-routes").innerHTML = "";

    // Clear nearby buses section
    document.getElementById("b").innerHTML = "";
    document.getElementById("nearby-buses").innerHTML = "";

    // Reset the map to default view
    initializeMap();
}

