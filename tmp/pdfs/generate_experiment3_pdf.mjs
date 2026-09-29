import fs from "node:fs";
import path from "node:path";
import { spawnSync } from "node:child_process";
import { marked } from "file:///C:/Users/34844/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/marked/lib/marked.esm.js";

const source = "D:/学习通/cxdownload/软件应用开发实践/实验三/实验3-软件架构与界面设计.md";
const output = "D:/学习通/cxdownload/软件应用开发实践/实验三/实验3-软件架构与界面设计.pdf";
const htmlPath = "D:/my_game/tmp/pdfs/experiment3-report.html";
const sourceDir = path.dirname(source);

const mimeTypes = new Map([
  [".png", "image/png"],
  [".jpg", "image/jpeg"],
  [".jpeg", "image/jpeg"],
  [".svg", "image/svg+xml"],
]);

let markdown = fs.readFileSync(source, "utf8");
markdown = markdown.replace(/!\[([^\]]*)\]\(([^)]+)\)/g, (whole, alt, target) => {
  const imagePath = path.resolve(sourceDir, target.trim());
  if (!fs.existsSync(imagePath)) return whole;
  const mime = mimeTypes.get(path.extname(imagePath).toLowerCase());
  if (!mime) return whole;
  const data = fs.readFileSync(imagePath).toString("base64");
  return `![${alt}](data:${mime};base64,${data})`;
});

const body = marked.parse(markdown, { gfm: true });
const html = `<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<title>实验3-软件架构与界面设计</title>
<style>
  @page { size: A4; margin: 14mm 15mm 15mm; }
  * { box-sizing: border-box; }
  body { margin: 0; color: #171717; font-family: "Microsoft YaHei", "Noto Sans CJK SC", sans-serif; font-size: 10.5pt; line-height: 1.58; }
  h1 { margin: 0 0 12pt; text-align: center; font-size: 22pt; }
  h2 { margin: 16pt 0 8pt; font-size: 16pt; break-after: avoid; }
  h3 { margin: 14pt 0 7pt; font-size: 13.5pt; break-after: avoid; }
  h4 { margin: 11pt 0 5pt; font-size: 11.5pt; break-after: avoid; }
  p { margin: 5pt 0 8pt; }
  ul, ol { margin: 5pt 0 9pt; padding-left: 22pt; }
  li { margin: 2pt 0; }
  table { width: 100%; margin: 7pt 0 11pt; border-collapse: collapse; table-layout: fixed; font-size: 8.4pt; }
  th, td { border: 0.6pt solid #b8b8b8; padding: 4.5pt 5pt; vertical-align: middle; overflow-wrap: anywhere; }
  th { background: #f1f3f5; font-weight: 700; }
  tr { break-inside: avoid; }
  img { display: block; max-width: 100%; max-height: 218mm; width: auto; height: auto; margin: 9pt auto 6pt; break-inside: avoid; }
  em { color: #333; }
  code { font-family: Consolas, "Microsoft YaHei", monospace; font-size: 0.9em; }
  pre { margin: 7pt 0 10pt; padding: 8pt 10pt; background: #f6f8fa; border: 0.6pt solid #d8dee4; border-radius: 3pt; white-space: pre-wrap; break-inside: avoid; }
  pre code { font-size: 8.5pt; }
  blockquote { margin: 8pt 0; padding-left: 10pt; border-left: 3pt solid #c9c9c9; color: #444; }
  h3:nth-of-type(5) { break-before: page; }
</style>
</head>
<body>${body}</body>
</html>`;

fs.writeFileSync(htmlPath, html, "utf8");
const chrome = "C:/Program Files/Google/Chrome/Application/chrome.exe";
const result = spawnSync(chrome, [
  "--headless",
  "--disable-gpu",
  "--allow-file-access-from-files",
  "--no-pdf-header-footer",
  `--print-to-pdf=${output}`,
  new URL(`file:///${htmlPath.replaceAll("\\", "/")}`).href,
], { encoding: "utf8", timeout: 120000 });

if (result.status !== 0 || !fs.existsSync(output)) {
  process.stderr.write(result.stderr || "PDF generation failed\n");
  process.exit(result.status || 1);
}
process.stdout.write(output + "\n");
