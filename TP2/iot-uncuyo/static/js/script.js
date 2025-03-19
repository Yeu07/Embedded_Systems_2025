document.addEventListener("DOMContentLoaded", function () {
    const ldrValueSpan = document.getElementById("ldrValue");
    const alarmStatusSpan = document.getElementById("alarmStatus");
    const toggleButton = document.getElementById("toggleButton");
    
    let lecturaActiva = false;

    function actualizarDatos() {
        fetch("/ldr")
            .then(response => response.json())
            .then(data => {
                ldrValueSpan.textContent = data.ldr;
                alarmStatusSpan.textContent = data.alarma ? "Activada" : "Desactivada";
            })
            .catch(error => console.error("Error obteniendo datos:", error));
    }

    toggleButton.addEventListener("click", function () {
        lecturaActiva = !lecturaActiva;
        fetch("/toggle_lectura", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ estado: lecturaActiva })
        })
        .then(response => response.json())
        .then(data => {
            toggleButton.textContent = data.lectura ? "Desactivar Lectura" : "Activar Lectura";
        })
        .catch(error => console.error("Error cambiando estado:", error));
    });

    setInterval(actualizarDatos, 3000); // Actualiza cada 3 segundos
});