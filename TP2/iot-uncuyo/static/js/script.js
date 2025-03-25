document.addEventListener("DOMContentLoaded", function () {
    const ldrValueSpan = document.getElementById("ldrValue");
    const alarmStatusSpan = document.getElementById("alarmStatus");
    const toggleButton = document.getElementById("toggleButton");
    
    function actualizarDatos() {
        fetch("/ldr")
            .then(response => response.json())
            .then(data => {
                ldrValueSpan.textContent = data.lectura ? data.ldr : "--";
                alarmStatusSpan.textContent = data.alarma ? "Activada" : "Desactivada";
                toggleButton.textContent = data.lectura ? "Desactivar Lectura" : "Activar Lectura";
            })
            .catch(error => console.error("Error obteniendo datos:", error));
    }

    toggleButton.addEventListener("click", function () {
        fetch("/toggle_lectura", {
            method: "POST",
            headers: { "Content-Type": "application/json" }
        })
        .then(response => response.json())
        .then(data => {
            toggleButton.textContent = data.lectura ? "Desactivar Lectura" : "Activar Lectura";
            actualizarDatos(); // Actualizar inmediatamente después de cambiar el estado
        })
        .catch(error => console.error("Error cambiando estado:", error));
    });

    setInterval(actualizarDatos, 1000); // Actualiza cada 3 segundos

});
