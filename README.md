# Cozy Snake Puzzle

Cozy Snake in the browser.

## Run locally
Open `index.html` in a browser.

## GitHub Pages
This repo is ready for GitHub Pages (root folder).  
Just push to GitHub, then enable Pages for the `main` branch.

## Backend (Postgres)
The frontend calls a small API for name + best score.

Start locally:
```
cd server
npm install
export DATABASE_URL="postgres://user:pass@host:5432/dbname"
export PG_SSL="true"
npm start
```

Set the API URL in `index.html`:
```
const API_BASE = "https://YOUR_BACKEND_URL"
```
