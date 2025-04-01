document.getElementById('updateTime').addEventListener('click', () => {
    fetch('/update_time', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
            document.getElementById('message').textContent = data.message || data.error;
        });
});

document.getElementById('getEvents').addEventListener('click', () => {
    fetch('/get_events')
        .then(response => response.json())
        .then(data => {
            let eventsDiv = document.getElementById('events');
            eventsDiv.innerHTML = '<h2>Eventos:</h2>';
            data.forEach(event => {
                eventsDiv.innerHTML += `<p>Pin: ${event.pin}, Tiempo: ${event.readable_time}</p>`;
            });
        });
});

document.getElementById('clearEEPROM').addEventListener('click', () => {
    fetch('/clear_eeprom', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
            document.getElementById('message').textContent = data.message;
        });
});

function fetchSerialTime() {
    fetch('/get_serial_time')
        .then(response => response.json())
        .then(data => {
            if (data.time) {
                document.getElementById('serialTime').textContent = `Hora actual: ${data.time}`;
            } else {
                console.error('Error obteniendo la hora:', data.error);
            }
        })
        .catch(error => console.error('Error en la solicitud:', error));
}

// Llamar a la función cada segundo
setInterval(fetchSerialTime, 1000);
