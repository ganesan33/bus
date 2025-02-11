import math
import json
import firebase_admin
from firebase_admin import credentials, firestore
from flask import Flask, jsonify, render_template, request, session, redirect, url_for
import os

# Load Firebase credentials
cred = credentials.Certificate("serviceAccountKey.json")
firebase_admin.initialize_app(cred)
db = firestore.client()

app = Flask(__name__)
app.secret_key = '3b9f8d7a6c5e4f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d6e5f4a3b2c1d0e9f8' 
@app.route('/')
def home():
    return render_template('index.html')

# Haversine formula to calculate distance between two lat-lon points
def haversine(lat1, lon1, lat2, lon2):
    R = 6371  # Earth radius in km
    dlat = math.radians(lat2 - lat1)
    dlon = math.radians(lon2 - lon1)
    a = math.sin(dlat / 2) ** 2 + math.cos(math.radians(lat1)) * math.cos(math.radians(lat2)) * math.sin(dlon / 2) ** 2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return R * c  # Distance in km

# Function to find nearby bus sources within 5 km
def get_nearby_bus_sources(all_buses, target_lat, target_lon, radius=5):
    nearby_buses = []
    
    for bus in all_buses:
        if "src" in bus and bus["src"]:  # Check if bus has a valid source coordinate
            src_lat, src_lon = bus["src"]
            distance = haversine(target_lat, target_lon, src_lat, src_lon)
            if distance <= radius:
                nearby_buses.append({
                    "busNumber": bus["busNumber"],
                    "busName": bus["busName"],
                    "distance": round(distance, 2)
                })

    return nearby_buses

# Fetch a specific bus by number and find nearby bus sources
@app.route('/get/<bus_no>', methods=['GET'])
def get_bus(bus_no):
    try:
        doc_ref = db.collection('buses').document(bus_no)
        doc = doc_ref.get()

        if not doc.exists:
            return jsonify({"error": "Bus not found"}), 404

        bus_data = doc.to_dict()
        all_buses = [b.to_dict() for b in db.collection('buses').stream()]  # Get all bus data

        # Get nearby bus sources (5 km radius)
        nearby_buses = get_nearby_bus_sources(all_buses, bus_data["src"][0], bus_data["src"][1], radius=5)

        bus_data["nearby_buses"] = nearby_buses  # Attach nearby bus sources

        return jsonify(bus_data)
    except Exception as e:
        return jsonify({"error": str(e)}), 500
# Fetch all bus routes
@app.route('/get_all_routes', methods=['GET'])
def get_all_routes():
    try:
        buses = db.collection('buses').stream()
        bus_list = [{"busNumber": bus.id, **bus.to_dict()} for bus in buses]
        return jsonify(bus_list)
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route('/validate_login', methods=['POST'])
def validate_login():
    try:
        data = request.json
        username = data.get("username")
        password = data.get("password")

        if not username or not password:
            return jsonify({"error": "Username and password are required"}), 400

        # Fetch admin credentials from Firestore
        admin_doc = db.collection("admins").document(username).get()

        if not admin_doc.exists:
            return jsonify({"error": "User not found"}), 404

        admin_data = admin_doc.to_dict()
        stored_password = admin_data.get("password")

        if stored_password is None:
            return jsonify({"error": "Password field missing"}), 500

        # Validate credentials
        if password == stored_password:
            # Set session variables
            session['logged_in'] = True
            session['username'] = username
            return jsonify({"success": True, "redirect": "/admin"}), 200
        else:
            return jsonify({"error": "Invalid username or password"}), 401

    except Exception as e:
        return jsonify({"error": str(e)}), 500

# Route to render the all routes page
@app.route('/routes')
def routes_page():
    return render_template('routes.html')

@app.route('/about')
def about():
    return render_template('about.html')


@app.route('/login')
def login_page():
    return render_template('login.html')

@app.route('/loggingInfo')
def logging_info():
    docs = db.collection('27c').stream()

    buses = {}
    for doc in docs:
        buses[doc.id] = doc.to_dict()  # Store document ID and data

    return render_template('info.html', buses=buses)

@app.route('/admin')
def admin_page():
    # Check if user is logged in
    if not session.get('logged_in'):
        return redirect(url_for('login_page'))  # Redirect to login if not
    return render_template('admin.html')
@app.route('/logout')
def logout():
    session.pop('logged_in', None)
    session.pop('username', None)
    return redirect(url_for('home'))
if __name__ == '__main__':
    app.run(debug=True)



