function updateLED(pin, value) {
    fetch('/update_led', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `led=${pin}&brillo=${value}`
    }).then(response => {
        if (response.ok) {
            document.getElementById(`led${pin}Value`).innerText = value;
        }
    }).catch(error => console.error('Error:', error));
}

function togglePin13() {
    let button = document.getElementById('togglePin13');
    let currentState = document.getElementById('pin13Status').innerText;
    let newState = (currentState === 'Apagado') ? 'ON' : 'OFF';
    
    fetch('/toggle_pin13', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `estado=${newState}`
    }).then(response => {
        if (response.ok) {
            document.getElementById('pin13Status').innerText = (newState === 'ON') ? 'Encendido' : 'Apagado';
        }
    }).catch(error => console.error('Error:', error));
}

function fetchLDR() {
    fetch('/ldr')
    .then(response => response.json())
    .then(data => {
        if (data.ldr) {
            document.getElementById('ldrValue').innerText = `LDR: ${data.ldr}`;
        }
    }).catch(error => console.error('Error:', error));
}

setInterval(fetchLDR, 1000);
