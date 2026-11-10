from flask import Flask, request, jsonify
from flask_cors import CORS
import requests

app = Flask(__name__)
CORS(app)


@app.route("/api/gps", methods=["POST"])
def receive_gps():

    data = request.get_json(silent=True)

    if not data:
        return jsonify({
            "error": "JSON data is required"
        }), 400

    latitude = data.get("latitude")
    longitude = data.get("longitude")

    if latitude is None or longitude is None:
        return jsonify({
            "error": "Latitude and longitude are required"
        }), 400

    try:
        latitude = float(latitude)
        longitude = float(longitude)
    except (ValueError, TypeError):
        return jsonify({
            "error": "Latitude and longitude must be numbers"
        }), 400

    # Reverse geocoding
    url = "https://nominatim.openstreetmap.org/reverse"

    params = {
        "lat": latitude,
        "lon": longitude,
        "format": "jsonv2",
        "zoom": 18
    }

    headers = {
        "User-Agent": "GPS-Tracker-Project/1.0"
    }

    response = requests.get(
        url,
        params=params,
        headers=headers,
        timeout=10
    )

    if response.status_code != 200:
        return jsonify({
            "error": "Reverse geocoding failed"
        }), 502

    location_data = response.json()

    address = location_data.get(
        "display_name",
        "Address not found"
    )

    return jsonify({
        "success": True,
        "latitude": latitude,
        "longitude": longitude,
        "address": address
    })


if __name__ == "__main__":
    app.run(
        host="0.0.0.0",
        port=5000,
        debug=True
    )