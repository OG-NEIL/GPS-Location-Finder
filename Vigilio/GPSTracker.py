from flask import Flask, request, jsonify,  render_template
import requests 
from flask_cors import CORS

app = Flask(__name__)
CORS(app)


@app.route("/api/gps", methods=["POST","GET", ])
def receive_gps():
    if request.method == "GET":
        return jsonify({
            "message": "GPS endpoint is working. Send coordinates using POST."
        })
    data = request.get_json()

    latitude = data.get("latitude")
    longitude = data.get("longitude")

    if latitude is None or longitude is None:
        return jsonify({
            "error": "GPS coordinates are required"
        }), 400

    try:
        latitude = float(latitude)
        longitude = float(longitude)

    except (TypeError, ValueError):
        return jsonify({
            "error": "Invalid GPS coordinates"
        }), 400

    # Validate coordinates
    if not -90 <= latitude <= 90:
        return jsonify({
            "error": "Invalid latitude"
        }), 400

    if not -180 <= longitude <= 180:
        return jsonify({
            "error": "Invalid longitude"
        }), 400

    # Reverse geocoding
    geocoding_url = "https://nominatim.openstreetmap.org/reverse"

    params = {
        "format": "json",
        "lat": latitude,
        "lon": longitude,
        "zoom": 18,
        "addressdetails": 1
    }

    headers = {
        "User-Agent": "GPS-Tracker-App/1.0"
    }

    response = requests.get(
        geocoding_url,
        params=params,
        headers=headers,
        timeout=10
    )

    if response.status_code != 200:
        return jsonify({
            "error": "Unable to reverse geocode coordinates"
        }), 500

    location = response.json()

    return jsonify({
        "latitude": latitude,
        "longitude": longitude,
        "address": location.get("display_name"),
        "details": location.get("address", {})
    })


if __name__ == "__main__":
    app.run(
        host="0.0.0.0",
        port=5000,
        debug=True
    )

           