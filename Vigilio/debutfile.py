from flask import Flask, request, jsonify, render_template
import requests

app = Flask(__name__)


@app.route("/")
def home():
    return render_template("locationfinder2.html")


@app.route("/api/reverse-geocode", methods=["POST"])
def reverse_geocode():

    data = request.get_json()

    latitude = data.get("latitude")
    longitude = data.get("longitude")

    if latitude is None or longitude is None:
        return jsonify({
            "error": "Latitude and longitude are required"
        }), 400

    try:
        latitude = float(latitude)
        longitude = float(longitude)
    except ValueError:
        return jsonify({
            "error": "Latitude and longitude must be numbers"
        }), 400

    # Validate coordinate ranges
    if not -90 <= latitude <= 90:
        return jsonify({
            "error": "Invalid latitude"
        }), 400

    if not -180 <= longitude <= 180:
        return jsonify({
            "error": "Invalid longitude"
        }), 400

    # Call Nominatim
    url = "https://nominatim.openstreetmap.org/reverse"

    params = {
        "format": "json",
        "lat": latitude,
        "lon": longitude
    }

    headers = {
        "User-Agent": "MyLocationApp/1.0"
    }

    response = requests.get(
        url,
        params=params,
        headers=headers,
        timeout=10
    )

    if response.status_code != 200:
        return jsonify({
            "error": "Geocoding service failed"
        }), 500

    result = response.json()

    return jsonify({
        "address": result.get("display_name"),
        "details": result.get("address", {})
    })


if __name__ == "__main__":
    app.run(debug=True, host="127.0.0.1", port=5500)