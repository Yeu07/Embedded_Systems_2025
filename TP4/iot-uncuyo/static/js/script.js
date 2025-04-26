document.addEventListener('DOMContentLoaded', function() {
    const peopleCountElement = document.getElementById('people-count');
    const detectionIndicator = document.getElementById('detection-indicator');
    const alarmStatus = document.getElementById('alarm-status');
    const alarmText = document.getElementById('alarm-text');
    const resetButton = document.getElementById('reset-btn');
    
    let detectionTimeout;
    
    // Función para actualizar los datos
    function updateData() {
        fetch('/get_data')
            .then(response => response.json())
            .then(data => {
                peopleCountElement.textContent = data.count;
                
                // Actualizar indicador de detección
                if (data.last_detection && (Date.now() / 1000 - data.last_detection) < 2) {
                    detectionIndicator.classList.add('active');
                    clearTimeout(detectionTimeout);
                    detectionTimeout = setTimeout(() => {
                        detectionIndicator.classList.remove('active');
                    }, 1000);
                }
                
                // Actualizar estado de la alarma
                if (data.alarm) {
                    alarmStatus.classList.add('active');
                    alarmText.textContent = 'ACTIVA';
                } else {
                    alarmStatus.classList.remove('active');
                    alarmText.textContent = 'INACTIVA';
                }
            })
            .catch(error => console.error('Error:', error));
    }
    
    // Configurar intervalo para actualizar datos
    setInterval(updateData, 500);
    
    // Manejar el botón de reinicio
    resetButton.addEventListener('click', function() {
        fetch('/reset', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            }
        })
        .then(response => response.json())
        .then(data => {
            if (data.status === 'success') {
                peopleCountElement.textContent = '0';
            }
        })
        .catch(error => console.error('Error:', error));
    });
});