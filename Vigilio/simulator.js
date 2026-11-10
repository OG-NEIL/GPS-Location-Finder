async function sendLocation() {

    const latitude =
        document.getElementById("latitude").value;

    const longitude =
        document.getElementById("longitude").value;

    const result =
        document.getElementById("result");


    if (!latitude || !longitude) {

        result.innerHTML =
            "Please enter both latitude and longitude.";

        return;
    }


    result.innerHTML = "Sending GPS coordinates...";


    try {

        const response = await fetch(
            "https://unspoiled-pastime-reassign.ngrok-free.dev/api/gps",
            {
                method: "POST",

                headers: {
                    "Content-Type": "application/json"
                },

                body: JSON.stringify({
                    latitude: Number(latitude),
                    longitude: Number(longitude)
                })
            }
        );


        const data = await response.json();


        if (!response.ok) {

            result.innerHTML =
                `Error: ${data.error || "Request failed"}`;

            return;
        }


        result.innerHTML = `
            <h3>GPS Location Received</h3>

            <p>
                <strong>Latitude:</strong>
                ${data.latitude}
            </p>

            <p>
                <strong>Longitude:</strong>
                ${data.longitude}
            </p>

            <p>
                <strong>Address:</strong>
                ${data.address}
            </p>
        `;

    } catch (error) {

        console.error(error);

        result.innerHTML =
            "Could not connect to the Flask server.";
    }
}