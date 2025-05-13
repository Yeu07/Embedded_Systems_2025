const ctx = document.getElementById('lightChart').getContext('2d');
let chart = null;

async function fetchData() {
  const response = await fetch('/data');
  const json = await response.json();

  const timestamps = [];
  const values = [];

  let lastTime = null;
  const discontinuityThreshold = 10000; // 10 segundos

  json.forEach((entry) => {
    const time = new Date(entry[0]);
    const value = entry[1];

    if (lastTime && (time - lastTime) > discontinuityThreshold) {
      timestamps.push(time);
      values.push(null); // Discontinuidad
    }

    timestamps.push(time);
    values.push(value);
    lastTime = time;
  });

  return { timestamps, values };
}

async function drawChart() {
  const { timestamps, values } = await fetchData();

  if (!chart) {
    // Crear gráfico por primera vez
    chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: timestamps,
        datasets: [{
          label: 'Intensidad Luminosa',
          data: values,
          borderColor: 'orange',
          backgroundColor: 'rgba(255,165,0,0.2)',
          spanGaps: false,
          tension: 0.3
        }]
      },
      options: {
        responsive: true,
        scales: {
          x: {
            type: 'time',
            time: {
              unit: 'minute',
              tooltipFormat: 'HH:mm:ss',
              displayFormats: {
                second: 'HH:mm:ss',
                minute: 'HH:mm'
              }
            },
            title: {
              display: true,
              text: 'Hora (UTC-3)'
            },
            ticks: {
              autoSkip: true,
              maxTicksLimit: 10
            }
          },
          y: {
            title: {
              display: true,
              text: 'Intensidad (0-1023)'
            }
          }
        }
      }
    });
  } else {
    // Solo actualizar los datos
    chart.data.labels = timestamps;
    chart.data.datasets[0].data = values;
    chart.width = window.innerWidth * 0.8; // Ajustar el ancho del gráfico
    chart.update();
  }
}

// Ejecutar por primera vez
drawChart();

// Actualizar cada 5 segundos
setInterval(drawChart, 5000);
