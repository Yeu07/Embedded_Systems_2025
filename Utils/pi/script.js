function sendCommand(command) {
    fetch('/send', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ command })
    })
        .then(response => response.json())
        .then(data => {
            console.log('Comando enviado:', data);
        })
        .catch(error => {
            console.error('Error al enviar comando:', error);
        });
}

function fetchStatus() {
    fetch('/status')
        .then(response => response.json())
        .then(data => {
            const lines = data.log.reverse(); // Último primero
            for (let line of lines) {
                if (line.startsWith("STATUS,")) {
                    const parts = line.split(',');
                    if (parts.length === 4) {
                        document.getElementById("status-state").textContent = parts[1];
                        document.getElementById("status-level").textContent = parts[3];
                        break;
                    }
                } else if (line.startsWith("Status: Waiting, LED:")) {
                    const match = line.match(/Status: Waiting, LED: (\d+), Level: (\d+)/);
                    if (match) {
                        document.getElementById("status-state").textContent = "Waiting";
                        document.getElementById("status-led").textContent = match[1];
                        document.getElementById("status-level").textContent = match[2];
                        break;
                    }
                } else if (line.startsWith("Status: Ready, LED:")) {
                    const match = line.match(/Status: Ready, LED: (\d+), Level: (\d+)/);
                    if (match) {
                        document.getElementById("status-state").textContent = "Ready";
                        document.getElementById("status-led").textContent = match[1];
                        document.getElementById("status-level").textContent = match[2];
                        break;
                    }
                }
            }
        })
        .catch(error => {
            console.error('Error al obtener estado:', error);
        });
}


// Actualiza el log cada segundo
setInterval(fetchStatus, 1000);

// Carga inicial
document.addEventListener('DOMContentLoaded', fetchStatus);
