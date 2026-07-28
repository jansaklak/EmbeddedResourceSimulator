#!/usr/bin/env python3
import http.server
import socketserver
import json
import os
import subprocess
import urllib.parse
import sys
import webbrowser
import concurrent.futures

PORT = 5050
WORKSPACE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(WORKSPACE_DIR, "data")
WEB_DIR = os.path.join(WORKSPACE_DIR, "web")
MAIN_BIN = os.path.join(WORKSPACE_DIR, "main")

class EmbeddedSimulatorHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == '/api/files':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            files = []
            if os.path.exists(DATA_DIR):
                for f in sorted(os.listdir(DATA_DIR)):
                    if f.endswith('.dat') or f.endswith('.temp'):
                        fpath = os.path.join(DATA_DIR, f)
                        files.append({
                            'name': f,
                            'size': os.path.getsize(fpath)
                        })
            self.wfile.write(json.dumps(files).encode('utf-8'))
            return

        elif parsed.path == '/api/file-content':
            params = urllib.parse.parse_qs(parsed.query)
            filename = params.get('name', [''])[0]
            if not filename:
                self.send_error(400, "Missing name parameter")
                return
            filepath = os.path.join(DATA_DIR, filename)
            if os.path.exists(filepath) and os.path.isfile(filepath):
                self.send_response(200)
                self.send_header('Content-Type', 'text/plain; charset=utf-8')
                self.end_headers()
                with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                    self.wfile.write(f.read().encode('utf-8'))
            else:
                self.send_error(404, "File not found")
            return

        return super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        content_length = int(self.headers.get('Content-Length', 0))
        body_data = self.rfile.read(content_length).decode('utf-8') if content_length > 0 else '{}'
        try:
            req_json = json.loads(body_data)
        except Exception:
            req_json = {}

        if parsed.path == '/api/run-simulation':
            strategy = req_json.get('strategy', 1)
            is_random = req_json.get('random', False)

            if is_random:
                tasks = req_json.get('tasks', 20)
                hc = req_json.get('hc', 4)
                pe = req_json.get('pe', 4)
                ch = req_json.get('channels', 1)
                with_cost = 1 if req_json.get('withCost', True) else 0
                conditional = 1 if req_json.get('conditional', False) else 0

                cmd = [MAIN_BIN, '--export-json-rand', str(tasks), str(hc), str(pe), str(ch), str(with_cost), str(conditional), str(strategy)]
            else:
                filename = req_json.get('filename', 'graph20.dat')
                file_path = os.path.join(DATA_DIR, filename)
                cmd = [MAIN_BIN, '--export-json', file_path, str(strategy)]

            try:
                proc = subprocess.run(cmd, cwd=WORKSPACE_DIR, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=30)
                stdout = proc.stdout
                stderr = proc.stderr
                
                # Parse JSON output from stdout
                json_start = stdout.find('{')
                json_end = stdout.rfind('}')
                if json_start != -1 and json_end != -1 and json_end > json_start:
                    json_str = stdout[json_start:json_end+1]
                    sim_data = json.loads(json_str)
                    sim_data['log'] = stdout[:json_start] + stderr
                    
                    # Read Gantt file if exists
                    gantt_path = os.path.join(WORKSPACE_DIR, 'gantt_data.dat')
                    gantt_lines = []
                    if os.path.exists(gantt_path):
                        with open(gantt_path, 'r') as gf:
                            for line in gf:
                                parts = line.strip().split()
                                if len(parts) >= 4:
                                    gantt_lines.append({
                                        'taskLabel': parts[0],
                                        'unit': parts[1],
                                        'start': int(parts[2]),
                                        'end': int(parts[3])
                                    })
                    sim_data['ganttRaw'] = gantt_lines

                    self.send_response(200)
                    self.send_header('Content-Type', 'application/json')
                    self.end_headers()
                    self.wfile.write(json.dumps(sim_data).encode('utf-8'))
                else:
                    self.send_response(500)
                    self.send_header('Content-Type', 'application/json')
                    self.end_headers()
                    self.wfile.write(json.dumps({'error': 'Failed to parse JSON output', 'stdout': stdout, 'stderr': stderr}).encode('utf-8'))

            except Exception as e:
                self.send_response(500)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({'error': str(e)}).encode('utf-8'))
            return

        elif parsed.path == '/api/benchmark':
            filename = req_json.get('filename', 'graph20.dat')
            file_path = os.path.join(DATA_DIR, filename)
            strategies = [1, 2, 3, 5, 6, 7, 8]
            results = []

            def run_strat(strat):
                cmd = [MAIN_BIN, '--export-json', file_path, str(strat)]
                try:
                    proc = subprocess.run(cmd, cwd=WORKSPACE_DIR, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=10)
                    stdout = proc.stdout
                    json_start = stdout.find('{')
                    json_end = stdout.rfind('}')
                    if json_start != -1 and json_end != -1:
                        data = json.loads(stdout[json_start:json_end+1])
                        return {
                            'strategy': strat,
                            'criticalTime': data.get('criticalTime', 0),
                            'totalCost': data.get('totalCost', 0),
                            'hardwareCount': data.get('hardwareCount', 0)
                        }
                except Exception as e:
                    return {'strategy': strat, 'error': str(e)}
                return {'strategy': strat, 'error': 'Failed to parse JSON'}

            with concurrent.futures.ThreadPoolExecutor(max_workers=len(strategies)) as executor:
                results = list(executor.map(run_strat, strategies))

            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(results).encode('utf-8'))
            return

        elif parsed.path == '/api/save-file':
            filename = req_json.get('filename', '').strip()
            content = req_json.get('content', '')
            if not filename:
                self.send_error(400, "Filename required")
                return
            if not filename.endswith('.dat') and not filename.endswith('.temp'):
                filename += '.dat'
            filepath = os.path.join(DATA_DIR, filename)
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({'success': True, 'filename': filename}).encode('utf-8'))
            return

        self.send_error(404, "Endpoint not found")

def main():
    if not os.path.exists(MAIN_BIN):
        print("Kompilacja silnika C++...")
        subprocess.run(["make", "main"], cwd=WORKSPACE_DIR)

    os.chdir(WEB_DIR)
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), EmbeddedSimulatorHandler) as httpd:
        url = f"http://localhost:{PORT}"
        print(f"=====================================================")
        print(f" Embedded Resource Simulator Web GUI running at:")
        print(f" {url}")
        print(f"=====================================================")
        try:
            webbrowser.open(url)
        except Exception:
            pass
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server...")

if __name__ == '__main__':
    main()
