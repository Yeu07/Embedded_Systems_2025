document.getElementById("update-time").addEventListener("click", () => {
    fetch("/update_time", { method: "POST" })
        .then(response => response.json())
        .then(data => alert(`Hora actualizada: ${data.unix_time}`))
        .catch(error => console.error("Error actualizando hora:", error));
});

document.getElementById("retrieve-events").addEventListener("click", () => {
    fetch("/retrieve_events")
        .then(response => response.json())
        .then(data => {
            const eventsDiv = document.getElementById("events");
            eventsDiv.innerHTML = "<h3>Eventos almacenados:</h3>";
            data.events.forEach(event => {
                eventsDiv.innerHTML += `<p>${event}</p>`;
            });
        })
        .catch(error => console.error("Error obteniendo eventos:", error));
});

document.getElementById("clear-eeprom").addEventListener("click", () => {
    fetch("/clear_eeprom", { method: "POST" })
        .then(response => response.json())
        .then(data => alert(data.message))
        .catch(error => console.error("Error borrando EEPROM:", error));
});
