import { createServer } from "node:http";
import { readFile, stat } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const root = __dirname;
const port = 4173;

const mimeTypes = new Map([
  [".html", "text/html; charset=utf-8"],
  [".css", "text/css; charset=utf-8"],
  [".js", "text/javascript; charset=utf-8"],
  [".mjs", "text/javascript; charset=utf-8"],
  [".json", "application/json; charset=utf-8"],
  [".png", "image/png"],
  [".jpg", "image/jpeg"],
  [".jpeg", "image/jpeg"],
  [".gif", "image/gif"],
  [".svg", "image/svg+xml; charset=utf-8"],
  [".webp", "image/webp"],
  [".txt", "text/plain; charset=utf-8"],
  [".md", "text/markdown; charset=utf-8"]
]);

function safePathFromUrl(urlPath) {
  const decoded = decodeURIComponent(urlPath.split("?")[0]);
  const normalized = path.normalize(decoded).replace(/^([/\\])+/, "");
  return path.join(root, normalized);
}

function respond(res, status, body, type = "text/plain; charset=utf-8") {
  res.writeHead(status, {
    "Content-Type": type,
    "Cache-Control": "no-store"
  });
  res.end(body);
}

createServer(async (req, res) => {
  try {
    let requestedPath = req.url || "/";
    if (requestedPath === "/")
      requestedPath = "/references/figma-phase1/index.html";

    let filePath = safePathFromUrl(requestedPath);
    if (!filePath.startsWith(root)) {
      respond(res, 403, "Forbidden");
      return;
    }

    let info = await stat(filePath).catch(() => null);
    if (info && info.isDirectory()) {
      filePath = path.join(filePath, "index.html");
      info = await stat(filePath).catch(() => null);
    }

    if (!info) {
      respond(res, 404, "Not found");
      return;
    }

    const ext = path.extname(filePath).toLowerCase();
    const mime = mimeTypes.get(ext) || "application/octet-stream";
    const body = await readFile(filePath);

    res.writeHead(200, {
      "Content-Type": mime,
      "Cache-Control": "no-store"
    });
    res.end(body);
  } catch (error) {
    respond(res, 500, `Server error\n${String(error)}`);
  }
}).listen(port, "127.0.0.1", () => {
  console.log(`stygian-builddoc server running at http://127.0.0.1:${port}/`);
});
