from fastapi import FastAPI, Form, Request
from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates
from fastapi.responses import FileResponse
from fastapi.security import APIKeyHeader
import csv
import os
from datetime import datetime
import dotenv

dotenv.load_dotenv()


app = FastAPI()
templates = Jinja2Templates(directory="static")
api_key_header_scheme = APIKeyHeader(name="X-API-Key", auto_error=True)

VALID_API_KEY = os.getenv("API_KEY")
CSV_FILE = "data/noise_data.csv"

# Ensure CSV exists
if not os.path.exists(CSV_FILE):
    with open(CSV_FILE, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["Timestamp", "Noise_Level"])


async def get_api_key(x_api_key: Annotated[str, Depends(api_key_header_scheme)]):
    if x_api_key != VALID_API_KEY:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid API Key"
        )
    return x_api_key

@app.get("/health")
async def health_check():
    return {
        "status": "online",
        "timestamp": datetime.now().isoformat(),
        "storage_file": CSV_FILE,
        "storage_size": f"{os.path.getsize(CSV_FILE) / 1024:.2f} KB"
    }

@app.post("/log", dependencies=[Depends(get_api_key)])
async def log_noise(level: int = Form(...)):
    timestamp = datetime.now().strftime("%H:%M:%S")
    with open(CSV_FILE, "a", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([timestamp, level])
    return {"status": "success"}

@app.get("/log", response_class=HTMLResponse)
async def get_dashboard(request: Request):
    # Read the last 20 entries for the graph
    labels = []
    values = []
    with open(CSV_FILE, "r") as f:
        rows = list(csv.reader(f))[1:] # Skip header
        for row in rows[-20:]: # Get last 20
            labels.append(row[0])
            values.append(int(row[1]))
            
    return templates.TemplateResponse("index.html", {
        "request": request, 
        "labels": labels, 
        "values": values
    })

@app.get("/download_data", response_class=HTMLResponse)
async def get_data(request: Request):
    file_path = CSV_FILE
    return FileResponse(
            file_path, 
            media_type='application/octet-stream', 
            filename="sound_data.csv"
    )
    
    

if __name__ == "__main__":
    import uvicorn
    # Ensure the 'templates' folder exists before running!
    uvicorn.run(app, host="0.0.0.0", port=8000)