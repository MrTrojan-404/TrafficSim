from flask import Flask, request, jsonify, render_template_string
import csv
import os
from datetime import datetime

app = Flask(__name__)

# Complete 7-variable dictionary
latest_data = {"active_cars": 0, "roadblocks": 0, "congestion": 0.0, "average_time": 0.0, "emissions": 0.0, "throughput": 0, "spawn_rate": 0}

run_timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
CSV_FILENAME = f"urbanflow_log_{run_timestamp}.csv"

# 1. Create the CSV file and write headers (Now includes Spawn_Rate_CPM)
if not os.path.exists(CSV_FILENAME):
    with open(CSV_FILENAME, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['Timestamp', 'Active_Cars', 'Roadblocks', 'Congestion_Percent', 'Average_Time_Sec', 'Emissions_Est', 'Throughput_CPM', 'Spawn_Rate_CPM'])

# 2. Update the telemetry route
@app.route('/telemetry', methods=['POST'])
def receive_telemetry():
    global latest_data
    incoming = request.json
    
    # Safely extract data
    latest_data["active_cars"] = incoming.get("active_cars", 0)
    latest_data["roadblocks"] = incoming.get("roadblocks", 0)
    latest_data["congestion"] = incoming.get("congestion", 0.0)
    latest_data["average_time"] = incoming.get("average_time", 0.0)
    latest_data["emissions"] = incoming.get("emissions", 0.0)
    latest_data["throughput"] = incoming.get("throughput", 0) 
    latest_data["spawn_rate"] = incoming.get("spawn_rate", 0)
    
    # Log ALL data to the CSV
    with open(CSV_FILENAME, mode='a', newline='') as file:
        writer = csv.writer(file)
        writer.writerow([
            datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            latest_data["active_cars"],
            latest_data["roadblocks"],
            round(latest_data["congestion"], 2),
            round(latest_data["average_time"], 2),
            round(latest_data["emissions"], 2),
            latest_data["throughput"],
            latest_data["spawn_rate"]
        ])
    
    return jsonify({"status": "success"})

@app.route('/data', methods=['GET'])
def get_data():
    return jsonify(latest_data)

@app.route('/')
def index():
    html = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>TrafficSim Command Center</title>
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <style>
            body { 
                background-color: #121212; 
                color: white; 
                font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
                text-align: center; 
                margin: 0;
                padding: 20px;
            }
            h2 {
                color: #ffffff;
                font-weight: 300;
                letter-spacing: 2px;
                margin-bottom: 30px;
            }
            /* Flexible grid so all 6 graphs fit beautifully! */
           .dashboard-grid { 
                display: grid; 
                grid-template-columns: repeat(3, 1fr); 
                gap: 20px; 
                width: 95%; 
                margin: auto; 
            }
            .chart-card { 
                background: #1e1e1e; 
                padding: 20px; 
                border-radius: 12px; 
                box-shadow: 0 4px 6px rgba(0,0,0,0.3);
            }
        </style>
    </head>
    <body>
        <h2>TRAFFICSIM COMMAND CENTER</h2>
        
        <div class="dashboard-grid">
            <div class="chart-card"><canvas id="carsChart"></canvas></div>
            <div class="chart-card"><canvas id="congestionChart"></canvas></div>
            <div class="chart-card"><canvas id="timeChart"></canvas></div>
            <div class="chart-card"><canvas id="emissionsChart"></canvas></div>
            <div class="chart-card"><canvas id="throughputChart"></canvas></div>
            <div class="chart-card"><canvas id="spawnChart"></canvas></div>
        </div>

        <script>
            function createChart(ctxId, labelText, lineColor) {
                var ctx = document.getElementById(ctxId).getContext('2d');
                return new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: [], 
                        datasets: [{
                            label: labelText,
                            borderColor: lineColor,
                            backgroundColor: lineColor + '33',
                            data: [],
                            borderWidth: 2,
                            tension: 0.4,
                            fill: true
                        }]
                    },
                    options: { 
                        animation: false, 
                        scales: { 
                            y: { beginAtZero: true, grid: { color: '#333333' } },
                            x: { grid: { display: false } }
                        },
                        plugins: {
                            legend: { labels: { color: 'white', font: { size: 14 } } }
                        }
                    }
                });
            }

            // Graphs
            var carsChart = createChart('carsChart', 'Active Vehicles', '#00d2ff'); 
            var congestionChart = createChart('congestionChart', 'Grid Congestion (%)', '#ff3b30'); 
            var timeChart = createChart('timeChart', 'Avg Travel Time (sec)', '#34c759'); 
            var emissionsChart = createChart('emissionsChart', 'Est. CO2 Emissions', '#ffcc00'); 
            var throughputChart = createChart('throughputChart', 'Grid Throughput (Out)', '#b066ff');
            var spawnChart = createChart('spawnChart', 'Grid Load (In)', '#007aff');

            function updateChart(chart, newData, timeLabel) {
                chart.data.labels.push(timeLabel);
                chart.data.datasets[0].data.push(newData);

                if (chart.data.labels.length > 40) {
                    chart.data.labels.shift();
                    chart.data.datasets[0].data.shift();
                }
                chart.update();
            }

            setInterval(() => {
                fetch('/data')
                    .then(response => response.json())
                    .then(data => {
                        let time = new Date().toLocaleTimeString();
                        
                        updateChart(carsChart, data.active_cars, time);
                        updateChart(congestionChart, data.congestion, time);
                        updateChart(timeChart, data.average_time, time);
                        updateChart(emissionsChart, data.emissions, time);
                        updateChart(throughputChart, data.throughput, time);
                        updateChart(spawnChart, data.spawn_rate, time);
                    });
            }, 500);
        </script>
    </body>
    </html>
    """
    return render_template_string(html)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
