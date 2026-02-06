const express = require("express");
const cors = require("cors");
const { Pool } = require("pg");

const app = express();
const PORT = process.env.PORT || 3000;
const DATABASE_URL = process.env.DATABASE_URL;

if (!DATABASE_URL) {
  console.error("Missing DATABASE_URL");
  process.exit(1);
}

const pool = new Pool({
  connectionString: DATABASE_URL,
  ssl: process.env.PG_SSL === "true" ? { rejectUnauthorized: false } : undefined
});

app.use(cors({ origin: true }));
app.use(express.json());

async function init() {
  await pool.query(
    `CREATE TABLE IF NOT EXISTS scores (
      name TEXT PRIMARY KEY,
      best INTEGER NOT NULL DEFAULT 0,
      updated_at TIMESTAMPTZ NOT NULL
    )`
  );
}

function normalizeName(name) {
  return String(name || "")
    .trim()
    .slice(0, 24);
}

app.get("/health", (req, res) => {
  res.json({ ok: true });
});

app.get("/leaderboard", async (req, res) => {
  const limit = Math.min(Number(req.query.limit) || 10, 50);
  try {
    const { rows } = await pool.query(
      `SELECT name, best, updated_at
       FROM scores
       ORDER BY best DESC, updated_at DESC
       LIMIT $1`,
      [limit]
    );
    res.json({ items: rows });
  } catch (err) {
    res.status(500).json({ error: "db_error" });
  }
});

app.get("/best", async (req, res) => {
  const name = normalizeName(req.query.name);
  if (!name) {
    res.status(400).json({ error: "name_required" });
    return;
  }
  try {
    const { rows } = await pool.query(
      `SELECT name, best, updated_at FROM scores WHERE name = $1`,
      [name]
    );
    res.json({ item: rows[0] || null });
  } catch (err) {
    res.status(500).json({ error: "db_error" });
  }
});

app.post("/score", async (req, res) => {
  const name = normalizeName(req.body?.name);
  const score = Number(req.body?.score || 0);
  if (!name) {
    res.status(400).json({ error: "name_required" });
    return;
  }
  if (!Number.isFinite(score) || score < 0) {
    res.status(400).json({ error: "score_invalid" });
    return;
  }

  const now = new Date().toISOString();

  try {
    const { rows } = await pool.query(
      `INSERT INTO scores (name, best, updated_at)
       VALUES ($1, $2, $3)
       ON CONFLICT(name) DO UPDATE SET
         best = GREATEST(scores.best, EXCLUDED.best),
         updated_at = EXCLUDED.updated_at
       RETURNING name, best, updated_at`,
      [name, Math.floor(score), now]
    );
    res.json(rows[0]);
  } catch (err) {
    res.status(500).json({ error: "db_error" });
  }
});

init()
  .then(() => {
    app.listen(PORT, () => {
      console.log(`Cozy Snake API listening on ${PORT}`);
    });
  })
  .catch((err) => {
    console.error("Failed to init DB", err);
    process.exit(1);
  });
