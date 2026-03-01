from fastapi import FastAPI, Form, Request
from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates
import csv
import os
from datetime import datetime

app = FastAPI()
templates = Jinja2Templates(directory="templates")
CSV_FILE = "noise_data.csv"

# Ensure CSV exists
if not os.path.exists(CSV_FILE):
    with open(CSV_FILE, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["Timestamp", "Noise_Level"])

@app.get("/health")
async def health_check():
    """Simple endpoint to verify the server is up and reachable."""
    return {
        "status": "online",
        "timestamp": datetime.now().isoformat(),
        "storage_file": CSV_FILE,
        "storage_size": f"{os.path.getsize(CSV_FILE) / 1024:.2f} KB"
    }

@app.post("/log")
async def log_noise(level: int = Form(...)):
    timestamp = datetime.now().strftime("%H:%M:%S")
    with open(CSV_FILE, "a", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([timestamp, level])
    return {"status": "success"}

@app.get("/dashboard", response_class=HTMLResponse)
async def get_dashboard(request: Request):
    # Read the last 20 entries for the graph
    labels = []
    values = []
    with open(CSV_FILE, "r") as f:
        rows = list(csv.reader(f))[1:] # Skip header
        for row in rows[-20:]: # Get last 20
            labels.append(row[0])
            values.append(int(row[1]))
            
    return templates.TemplateResponse("dashboard.html", {
        "request": request, 
        "labels": labels, 
        "values": values
    })

if __name__ == "__main__":
    import uvicorn
    # Ensure the 'templates' folder exists before running!
    uvicorn.run(app, host="0.0.0.0", port=5000)