import cv2
import numpy as np
import yaml
import logging
import time
import os
import pygame
import json
import requests
import threading
import queue
from datetime import datetime
from ultralytics import YOLO
from typing import List, Dict, Any, Optional
from dataclasses import dataclass
from io import BytesIO

@dataclass
class SceneAnalysis:
    threat_level: str
    description: str
    recommendations: List[str]
    confidence: float
    reasoning: str

class LocalLLMAnalyzer:
    def __init__(self, config: dict):
        self.config = config
        self.llm_config = config.get('llm', {})
        self.enabled = self.llm_config.get('enabled', True)
        self.ollama_url = self.llm_config.get('ollama_url', 'http://localhost:11434')
        self.model = self.llm_config.get('model', 'llama2:7b')
        self.analysis_queue = queue.Queue(maxsize=10)
        self.result_queue = queue.Queue()
        self.running = False
        if self.enabled:
            self.start_analyzer()
    
    def start_analyzer(self):
        if self.check_ollama_connection():
            self.running = True
            threading.Thread(target=self._analysis_worker, daemon=True).start()
            logging.info("LLM analyzer started")
        else:
            logging.warning("Ollama not available")
            self.enabled = False
    
    def stop_analyzer(self):
        self.running = False
    
    def check_ollama_connection(self) -> bool:
        try:
            response = requests.get(f"{self.ollama_url}/api/tags", timeout=5)
            return response.status_code == 200
        except:
            return False
    
    def analyze_scene_async(self, detections: List[Dict], frame_info: Dict):
        if not self.enabled:
            return
        try:
            self.analysis_queue.put_nowait({'detections': detections, 'frame_info': frame_info, 'timestamp': time.time()})
        except queue.Full:
            pass
    
    def get_latest_analysis(self) -> Optional[SceneAnalysis]:
        try:
            return self.result_queue.get_nowait()
        except queue.Empty:
            return None
    
    def _analysis_worker(self):
        while self.running:
            try:
                data = self.analysis_queue.get(timeout=0.5)
                result = self._analyze_scene_with_llm(data['detections'], data['frame_info'])
                try:
                    self.result_queue.get_nowait()
                except queue.Empty:
                    pass
                self.result_queue.put_nowait(result)
            except queue.Empty:
                continue
            except Exception as e:
                logging.error(f"Analysis error: {e}")
    
    def _analyze_scene_with_llm(self, detections: List[Dict], frame_info: Dict) -> SceneAnalysis:
        try:
            prompt = f"Analyze security scene: {len(detections)} objects detected. Respond in JSON with threat_level, description, recommendations, reasoning."
            response = self._query_ollama(prompt)
            return self._parse_llm_response(response, detections)
        except:
            return self._fallback_analysis(detections)
    
    def _query_ollama(self, prompt: str) -> str:
        try:
            payload = {"model": self.model, "prompt": prompt, "stream": False, "options": {"temperature": 0.3}}
            response = requests.post(f"{self.ollama_url}/api/generate", json=payload, timeout=30)
            if response.status_code == 200:
                return response.json().get('response', '')
        except:
            pass
        return ""
    
    def _parse_llm_response(self, response: str, detections: List[Dict]) -> SceneAnalysis:
        try:
            start = response.find('{')
            end = response.rfind('}') + 1
            if start >= 0 and end > start:
                data = json.loads(response[start:end])
                return SceneAnalysis(
                    threat_level=data.get('threat_level', 'low'),
                    description=data.get('description', ''),
                    recommendations=data.get('recommendations', []),
                    confidence=float(data.get('confidence', 0.5)),
                    reasoning=data.get('reasoning', '')
                )
        except:
            pass
        return self._fallback_analysis(detections)
    
    def _fallback_analysis(self, detections: List[Dict]) -> SceneAnalysis:
        dangerous = [d for d in detections if d.get('is_dangerous', False)]
        if dangerous:
            level = "high" if len(dangerous) > 1 else "medium"
        else:
            level = "low"
        return SceneAnalysis(threat_level=level, description="Analysis complete", recommendations=[], confidence=0.8, reasoning="Fallback")

class SmartReportGenerator:
    def __init__(self):
        self.incident_history = []
    
    def generate_incident_report(self, analysis: SceneAnalysis, detections: List[Dict], timestamp: str) -> Dict:
        report = {
            'timestamp': timestamp,
            'threat_level': analysis.threat_level,
            'summary': analysis.description,
            'recommendations': analysis.recommendations,
            'incident_id': f"INC_{int(time.time())}"
        }
        self.incident_history.append(report)
        return report

class AlertSystem:
    def __init__(self, config: dict):
        self.config = config
        self.alert_config = config['alerts']
        self.last_alert_time = 0
        self.alert_duration = self.alert_config.get('alert_duration', 3.0)
        self.alert_queue = queue.Queue()
        self.alert_history = []
        self.alert_sounds = {}
        self.running = False
        if self.alert_config.get('enable_audio', True):
            self.init_audio()
        self.create_alert_sounds()
    
    def init_audio(self):
        try:
            pygame.mixer.init(frequency=22050, size=-16, channels=2, buffer=512)
            logging.info("Audio initialized")
        except Exception as e:
            logging.error(f"Audio init failed: {e}")
    
    def create_alert_sounds(self):
        if not self.alert_config.get('enable_audio', True):
            return
        try:
            self.alert_sounds['high'] = self.generate_alert_sound(800, 0.5)
            self.alert_sounds['medium'] = self.generate_alert_sound(600, 0.3)
            self.alert_sounds['low'] = self.generate_alert_sound(400, 0.2)
        except Exception as e:
            logging.error(f"Sound creation failed: {e}")
    
    def generate_alert_sound(self, frequency: int, duration: float) -> Optional[pygame.mixer.Sound]:
        try:
            sample_rate = 22050
            frames = int(duration * sample_rate)
            arr = np.sin(2 * np.pi * frequency * np.arange(frames) / sample_rate) * 0.5
            arr = (arr * 32767).astype(np.int16)
            stereo = np.zeros((frames, 2), dtype=np.int16)
            stereo[:, 0] = arr
            stereo[:, 1] = arr
            return pygame.sndarray.make_sound(stereo)
        except:
            return None
    
    def start(self):
        self.running = True
        threading.Thread(target=self._alert_worker, daemon=True).start()
        logging.info("Alert system started")
    
    def stop(self):
        self.running = False
    
    def trigger_alert(self, detections: List[Dict], frame: Optional[np.ndarray] = None):
        if not detections:
            return
        current_time = time.time()
        if current_time - self.last_alert_time < self.alert_duration:
            return
        threat_level = self._assess_threat_level(detections)
        if threat_level == 'none':
            return
        try:
            self.alert_queue.put_nowait({'timestamp': current_time, 'threat_level': threat_level, 'detections': detections, 'frame': frame})
            self.last_alert_time = current_time
        except queue.Full:
            pass
    
    def _assess_threat_level(self, detections: List[Dict]) -> str:
        max_score = max([d.get('weapon_score', 0) for d in detections if d.get('is_dangerous', False)], default=0)
        dangerous_count = len([d for d in detections if d.get('is_dangerous', False)])
        if max_score > 0.8 or dangerous_count > 2:
            return 'high'
        elif max_score > 0.5 or dangerous_count > 0:
            return 'medium'
        elif max_score > 0.3:
            return 'low'
        return 'none'
    
    def _alert_worker(self):
        while self.running:
            try:
                alert_data = self.alert_queue.get(timeout=0.5)
                self._process_alert(alert_data)
            except queue.Empty:
                continue
            except Exception as e:
                logging.error(f"Alert worker error: {e}")
    
    def _process_alert(self, alert_data: Dict):
        self._play_audio_alert(alert_data['threat_level'])
        self._add_to_history(alert_data)
        self._emergency_notification(alert_data)
        logging.warning(f"ALERT: {alert_data['threat_level'].upper()}")
    
    def _play_audio_alert(self, threat_level: str):
        try:
            sound = self.alert_sounds.get(threat_level)
            if sound:
                sound.play()
        except:
            pass
    
    def _emergency_notification(self, alert_data: Dict):
        threat_level = alert_data['threat_level']
        timestamp = datetime.fromtimestamp(alert_data['timestamp']).strftime('%Y-%m-%d %H:%M:%S')
        detection_count = len(alert_data['detections'])
        
        if threat_level == 'high':
            message = f"🚨 CRITICAL SECURITY ALERT 🚨\nTime: {timestamp}\nThreat Level: HIGH\nDetections: {detection_count}\nACTION REQUIRED: Immediate response needed"
            logging.critical(message)
        elif threat_level == 'medium':
            message = f"⚠️ SECURITY ALERT\nTime: {timestamp}\nThreat Level: MEDIUM\nDetections: {detection_count}\nACTION: Monitor and assess situation"
            logging.warning(message)
    
    def _add_to_history(self, alert_data: Dict):
        self.alert_history.append({'timestamp': alert_data['timestamp'], 'threat_level': alert_data['threat_level']})
        if len(self.alert_history) > 100:
            self.alert_history.pop(0)
    
    def get_recent_alerts(self, count: int = 10) -> List[Dict]:
        return self.alert_history[-count:] if self.alert_history else []
    
    def draw_alert_overlay(self, frame: np.ndarray, detections: List[Dict]) -> np.ndarray:
        overlay = frame.copy()
        height, width = frame.shape[:2]
        dangerous = [d for d in detections if d.get('is_dangerous', False)]
        if dangerous:
            if int(time.time() * 4) % 2:
                overlay[:] = cv2.addWeighted(overlay, 0.8, np.full_like(overlay, (0, 0, 255)), 0.2, 0)
            cv2.putText(overlay, "DANGER DETECTED", (width//3, 60), cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 0, 255), 3)
        return overlay

class RemoteVideoStream:
    """Handles streaming from ESP32-CAM or other remote video sources"""
    def __init__(self, stream_url: str, reconnect_attempts: int = 5):
        self.stream_url = stream_url
        self.reconnect_attempts = reconnect_attempts
        self.running = False
        self.frame_buffer = queue.Queue(maxsize=5)  # Increased buffer size
        self.current_frame = None
        self.stream_thread = None
        self.connection_active = False
        self.frame_timeout = 0.1  # Short timeout for read attempts
        
    def start(self):
        """Start the video stream thread"""
        self.running = True
        self.stream_thread = threading.Thread(target=self._stream_worker, daemon=True)
        self.stream_thread.start()
        logging.info(f"Remote video stream started: {self.stream_url}")
    
    def stop(self):
        """Stop the video stream"""
        self.running = False
        if self.stream_thread:
            self.stream_thread.join(timeout=5)
        logging.info("Remote video stream stopped")
    
    def read(self) -> tuple:
        """Read a frame from the stream (compatible with cv2.VideoCapture interface)"""
        try:
            # Try to get a new frame with short timeout
            self.current_frame = self.frame_buffer.get(timeout=self.frame_timeout)
            return True, self.current_frame
        except queue.Empty:
            # Return last frame if available (keep stream alive)
            if self.current_frame is not None:
                return True, self.current_frame
            # Only fail if we have no frame at all
            return False, None
    
    def _stream_worker(self):
        """Worker thread for streaming video"""
        retry_count = 0
        
        while self.running:
            try:
                logging.info(f"Connecting to stream... (attempt {retry_count + 1}/{self.reconnect_attempts})")
                logging.info(f"Stream URL: {self.stream_url}")
                response = requests.get(self.stream_url, stream=True, timeout=(10, 30))
                bytes_data = b''
                retry_count = 0
                self.connection_active = True
                logging.info("Stream connection established successfully")
                
                while self.running:
                    try:
                        for chunk in response.iter_content(chunk_size=4096):
                            if not self.running:
                                break
                            
                            if chunk:
                                bytes_data += chunk
                                
                                # Find JPEG frame boundaries
                                a = bytes_data.find(b'\xff\xd8')  # JPEG start
                                b = bytes_data.find(b'\xff\xd9')  # JPEG end
                                
                                if a != -1 and b != -1 and b > a:
                                    jpg = bytes_data[a:b+2]
                                    bytes_data = bytes_data[b+2:]
                                    
                                    # Validate JPEG
                                    if len(jpg) > 100:
                                        # Decode frame
                                        frame = cv2.imdecode(np.frombuffer(jpg, dtype=np.uint8), cv2.IMREAD_COLOR)
                                        
                                        if frame is not None:
                                            # Put frame in buffer (discard old frames if buffer full)
                                            try:
                                                self.frame_buffer.put_nowait(frame)
                                            except queue.Full:
                                                try:
                                                    self.frame_buffer.get_nowait()
                                                    self.frame_buffer.put_nowait(frame)
                                                except:
                                                    pass
                    
                    except (ConnectionError, ConnectionResetError, BrokenPipeError):
                        logging.warning("Connection lost, reconnecting...")
                        self.connection_active = False
                        break
            
            except requests.exceptions.Timeout:
                retry_count += 1
                self.connection_active = False
                logging.warning(f"Timeout, retrying... ({retry_count}/{self.reconnect_attempts})")
                time.sleep(2)
            
            except requests.exceptions.ConnectionError:
                retry_count += 1
                self.connection_active = False
                logging.warning(f"Connection failed, retrying... ({retry_count}/{self.reconnect_attempts})")
                time.sleep(2)
            
            except Exception as e:
                retry_count += 1
                self.connection_active = False
                logging.error(f"Stream error: {e}")
                time.sleep(2)
            
            if retry_count >= self.reconnect_attempts and self.running:
                logging.error("Max retries reached for remote stream")
                self.running = False
                break
    
    def isOpened(self) -> bool:
        """Check if stream is active (compatible with cv2.VideoCapture interface)"""
        return self.connection_active
    
    def set(self, prop_id: int, value: float) -> bool:
        """Dummy method for compatibility with cv2.VideoCapture interface"""
        return True
    
    def release(self):
        """Release the stream"""
        self.stop()


class WeaponDetector:
    def __init__(self, config: dict):
        self.config = config
        self.primary_model = None
        self.secondary_model = None
        self.load_models()
        self.danger_threshold = 0.2
    
    def load_models(self):
        try:
            if os.path.exists('weapon_detection_custom.pt'):
                self.primary_model = YOLO('weapon_detection_custom.pt')
                self.secondary_model = YOLO('weapon_detection_custom.pt')
                logging.info("Custom model loaded")
            else:
                self.primary_model = YOLO('yolov8n.pt')
                self.secondary_model = YOLO('yolov8s.pt')
                logging.info("Generic YOLO models loaded")
        except Exception as e:
            logging.error(f"Model loading error: {e}")
            raise
    
    def detect_weapons(self, frame: np.ndarray) -> List[Dict]:
        detections = []
        try:
            results = self.primary_model(frame, conf=self.danger_threshold)
            for result in results:
                if result.boxes is not None:
                    for box in result.boxes:
                        x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                        confidence = float(box.conf[0].cpu().numpy())
                        class_id = int(box.cls[0].cpu().numpy())
                        class_name = self.primary_model.names[class_id]
                        weapon_score = self._calculate_weapon_score(class_name, confidence)
                        if weapon_score > 0:
                            detections.append({
                                'bbox': (int(x1), int(y1), int(x2), int(y2)),
                                'confidence': confidence,
                                'weapon_score': weapon_score,
                                'class_name': class_name,
                                'detection_method': 'primary_model',
                                'is_weapon': weapon_score > 0.5
                            })
        except Exception as e:
            logging.error(f"Weapon detection error: {e}")
        return detections
    
    def _calculate_weapon_score(self, class_name: str, confidence: float) -> float:
        class_lower = class_name.lower()
        if class_lower in ['knife', 'gun', 'pistol', 'rifle']:
            return confidence
        elif class_lower in ['scissors']:
            return confidence * 0.95
        elif class_lower in ['hammer', 'axe']:
            return confidence * 0.7
        return 0

class AIDetectionSystem:
    def __init__(self, config_path: str = "config.yaml"):
        self.config = self.load_config(config_path)
        self.setup_logging()
        
        self.human_model = None
        self.object_model = None
        self.weapon_model = None
        self.load_models()
        
        self.weapon_detector = WeaponDetector(self.config)
        self.alert_system = AlertSystem(self.config)
        self.llm_analyzer = LocalLLMAnalyzer(self.config)
        self.report_generator = SmartReportGenerator()
        
        self.camera = None
        self.setup_camera()
        
        self.last_llm_analysis = 0
        self.llm_analysis_interval = self.config.get('llm', {}).get('analysis_interval', 2.0)
        self.frame_count = 0
        self.start_time = time.time()
        
        os.makedirs(self.config['logging']['output_dir'], exist_ok=True)
    
    def load_config(self, config_path: str) -> Dict[str, Any]:
        try:
            with open(config_path, 'r') as file:
                return yaml.safe_load(file)
        except FileNotFoundError:
            logging.error(f"Config file {config_path} not found")
            return self.get_default_config()
    
    def get_default_config(self) -> Dict[str, Any]:
        return {
            'detection': {
                'confidence_threshold': 0.5,
                'nms_threshold': 0.4,
                'input_size': 640,
                'camera_index': 0,
                'frame_width': 1280,
                'frame_height': 720,
                'fps': 30,
                'human_model': 'yolov8n.pt',
                'object_model': 'yolov8n.pt',
                'weapon_model': 'yolov8n.pt',
                'use_remote_stream': True,
                'remote_stream_url': 'http://192.168.1.40:80/stream',
                'remote_reconnect_attempts': 5
            },
            'alerts': {
                'enable_audio': True,
                'enable_visual': True,
                'alert_duration': 3.0,
                'dangerous_objects': ['knife', 'gun', 'rifle', 'pistol', 'weapon']
            },
            'logging': {
                'level': 'INFO',
                'save_detections': True,
                'output_dir': 'detections'
            },
            'display': {
                'window_title': 'AI Security Detection System',
                'show_confidence': True,
                'show_fps': True,
                'bbox_thickness': 2,
                'font_scale': 0.6
            }
        }
    
    def setup_logging(self):
        log_level = getattr(logging, self.config['logging']['level'])
        logging.basicConfig(
            level=log_level,
            format='%(asctime)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler('detection.log'),
                logging.StreamHandler()
            ]
        )
    
    def load_models(self):
        try:
            logging.info("Loading detection models...")
            self.human_model = YOLO(self.config['detection']['human_model'])
            self.object_model = YOLO(self.config['detection']['object_model'])
            self.weapon_model = YOLO(self.config['detection']['weapon_model'])
            logging.info("Models loaded successfully")
        except Exception as e:
            logging.error(f"Error loading models: {e}")
            raise
    
    def setup_camera(self):
        try:
            use_remote = self.config['detection'].get('use_remote_stream', False)
            
            if use_remote:
                # Use remote ESP32-CAM stream
                stream_url = self.config['detection'].get('remote_stream_url', 'http://192.168.1.40:80/stream')
                reconnect_attempts = self.config['detection'].get('remote_reconnect_attempts', 5)
                self.camera = RemoteVideoStream(stream_url, reconnect_attempts)
                self.camera.start()
                logging.info(f"Remote camera stream initialized: {stream_url}")
                
                # Wait for connection to establish (up to 10 seconds)
                connection_timeout = 10
                start_time = time.time()
                while not self.camera.isOpened() and (time.time() - start_time) < connection_timeout:
                    time.sleep(0.5)
                    logging.info("Waiting for remote stream connection...")
                
                if not self.camera.isOpened():
                    raise RuntimeError(f"Could not connect to remote stream after {connection_timeout} seconds")
            else:
                # Use local camera
                self.camera = cv2.VideoCapture(self.config['detection']['camera_index'])
                self.camera.set(cv2.CAP_PROP_FRAME_WIDTH, self.config['detection']['frame_width'])
                self.camera.set(cv2.CAP_PROP_FRAME_HEIGHT, self.config['detection']['frame_height'])
                self.camera.set(cv2.CAP_PROP_FPS, self.config['detection']['fps'])
                logging.info("Local camera initialized")
                
                if not self.camera.isOpened():
                    raise RuntimeError("Could not open local camera")
            
            logging.info("Camera/Stream initialized successfully")
        except Exception as e:
            logging.error(f"Error initializing camera: {e}")
            raise
    
    def detect_objects(self, frame: np.ndarray) -> List[Dict]:
        detections = []
        try:
            results = self.object_model(frame, conf=self.config['detection']['confidence_threshold'])
            for result in results:
                if result.boxes is not None:
                    for box in result.boxes:
                        x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                        confidence = float(box.conf[0].cpu().numpy())
                        class_id = int(box.cls[0].cpu().numpy())
                        class_name = self.object_model.names[class_id]
                        detection_type = 'human' if class_name == 'person' else 'object'
                        detections.append({
                            'bbox': (int(x1), int(y1), int(x2), int(y2)),
                            'confidence': confidence,
                            'class_name': class_name,
                            'type': detection_type,
                            'is_dangerous': self.is_dangerous_object(class_name)
                        })
            
            weapon_detections = self.weapon_detector.detect_weapons(frame)
            for weapon_detection in weapon_detections:
                weapon_detection['is_dangerous'] = weapon_detection.get('is_weapon', False)
                detections.append(weapon_detection)
        except Exception as e:
            logging.error(f"Error during detection: {e}")
        
        return detections
    
    def is_dangerous_object(self, class_name: str) -> bool:
        dangerous_objects = self.config['alerts']['dangerous_objects']
        return any(dangerous_item.lower() in class_name.lower() for dangerous_item in dangerous_objects)
    
    def draw_detections(self, frame: np.ndarray, detections: List[Dict]) -> np.ndarray:
        for detection in detections:
            x1, y1, x2, y2 = detection['bbox']
            confidence = detection['confidence']
            class_name = detection['class_name']
            is_dangerous = detection.get('is_dangerous', False)
            
            color = (0, 0, 255) if is_dangerous else (0, 255, 0) if detection.get('type') == 'human' else (255, 0, 0)
            thickness = 3 if is_dangerous else 2
            
            cv2.rectangle(frame, (x1, y1), (x2, y2), color, thickness)
            
            label = f"{class_name}: {confidence:.2f}" if self.config['display']['show_confidence'] else class_name
            if is_dangerous:
                label = f"⚠️ {label} ⚠️"
            
            font_scale = self.config['display']['font_scale']
            (label_w, label_h), baseline = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, font_scale, 1)
            
            cv2.rectangle(frame, (x1, y1 - label_h - 10), (x1 + label_w + 10, y1), color, -1)
            cv2.putText(frame, label, (x1 + 5, y1 - 5), cv2.FONT_HERSHEY_SIMPLEX, font_scale, (255, 255, 255), 1)
        
        return frame
    
    def add_info_overlay(self, frame: np.ndarray) -> np.ndarray:
        height, width = frame.shape[:2]
        
        if self.config['display']['show_fps']:
            self.frame_count += 1
            elapsed_time = time.time() - self.start_time
            fps = self.frame_count / elapsed_time if elapsed_time > 0 else 0
            cv2.putText(frame, f"FPS: {fps:.1f}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        
        title = self.config['display']['window_title']
        cv2.putText(frame, title, (10, height - 20), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        
        cv2.circle(frame, (width - 30, 30), 10, (0, 255, 0), -1)
        
        return frame
    
    def run(self):
        logging.info("Starting AI Detection System...")
        logging.info("=" * 60)
        logging.info("SECURITY MONITORING ACTIVE - Life Safety Mode Enabled")
        logging.info("=" * 60)
        self.alert_system.start()
        
        consecutive_failures = 0
        max_consecutive_failures = 30  # Allow up to 30 consecutive failures before stopping
        
        try:
            while True:
                ret, frame = self.camera.read()
                if not ret or frame is None:
                    consecutive_failures += 1
                    if consecutive_failures > max_consecutive_failures:
                        logging.error(f"Failed to capture frame {consecutive_failures} times, stopping")
                        break
                    # Log but continue (don't break immediately)
                    logging.warning(f"Failed to capture frame (attempt {consecutive_failures}/{max_consecutive_failures})")
                    time.sleep(0.1)  # Brief pause before retry
                    continue
                
                # Reset failure counter on successful frame
                consecutive_failures = 0
                
                detections = self.detect_objects(frame)
                
                current_time = time.time()
                if current_time - self.last_llm_analysis > self.llm_analysis_interval:
                    self.last_llm_analysis = current_time
                    frame_info = {'fps': self.frame_count / (current_time - self.start_time) if current_time > self.start_time else 0, 'timestamp': datetime.now().strftime('%H:%M:%S')}
                    self.llm_analyzer.analyze_scene_async(detections, frame_info)
                
                latest_analysis = self.llm_analyzer.get_latest_analysis()
                if latest_analysis and latest_analysis.threat_level in ['high', 'critical']:
                    report = self.report_generator.generate_incident_report(latest_analysis, detections, datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
                    logging.critical(f"INCIDENT REPORT GENERATED: {report['incident_id']}")
                    self._save_incident_evidence(frame, detections, report)
                
                dangerous_detections = [d for d in detections if d.get('is_dangerous', False)]
                if dangerous_detections:
                    self.alert_system.trigger_alert(dangerous_detections, frame)
                    self._log_threat_details(dangerous_detections)
                
                frame = self.draw_detections(frame, detections)
                frame = self.alert_system.draw_alert_overlay(frame, detections)
                frame = self.add_info_overlay(frame)
                
                cv2.imshow(self.config['display']['window_title'], frame)
                
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    logging.info("Exit requested by user")
                    break
        except KeyboardInterrupt:
            logging.info("Detection stopped by user")
        except Exception as e:
            logging.error(f"Error in main loop: {e}")
        finally:
            self.cleanup()
    
    def _log_threat_details(self, dangerous_detections: List[Dict]):
        for detection in dangerous_detections:
            class_name = detection.get('class_name', 'unknown')
            confidence = detection.get('confidence', 0)
            weapon_score = detection.get('weapon_score', 0)
            bbox = detection.get('bbox', (0, 0, 0, 0))
            
            logging.critical(f"THREAT DETECTED: {class_name}")
            logging.critical(f"  Confidence: {confidence:.2%}")
            logging.critical(f"  Weapon Score: {weapon_score:.2%}")
            logging.critical(f"  Location: {bbox}")
    
    def _save_incident_evidence(self, frame: np.ndarray, detections: List[Dict], report: Dict):
        try:
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            evidence_dir = os.path.join(self.config['logging']['output_dir'], 'incidents')
            os.makedirs(evidence_dir, exist_ok=True)
            
            frame_path = os.path.join(evidence_dir, f"incident_{report['incident_id']}_frame.jpg")
            report_path = os.path.join(evidence_dir, f"incident_{report['incident_id']}_report.json")
            
            annotated = frame.copy()
            for detection in detections:
                if detection.get('is_dangerous', False):
                    x1, y1, x2, y2 = detection['bbox']
                    cv2.rectangle(annotated, (x1, y1), (x2, y2), (0, 0, 255), 3)
                    cv2.putText(annotated, f"THREAT: {detection['class_name']}", (x1, y1-10), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
            
            cv2.imwrite(frame_path, annotated)
            with open(report_path, 'w') as f:
                json.dump(report, f, indent=2)
            
            logging.critical(f"EVIDENCE SAVED: {frame_path}")
            logging.critical(f"REPORT SAVED: {report_path}")
        except Exception as e:
            logging.error(f"Failed to save incident evidence: {e}")
    
    def cleanup(self):
        logging.info("=" * 60)
        logging.info("SECURITY MONITORING SHUTDOWN")
        logging.info("=" * 60)
        if hasattr(self, 'alert_system'):
            self.alert_system.stop()
        if hasattr(self, 'llm_analyzer'):
            self.llm_analyzer.stop_analyzer()
        if self.camera:
            if isinstance(self.camera, RemoteVideoStream):
                self.camera.stop()
            else:
                self.camera.release()
        cv2.destroyAllWindows()
        logging.info("System safely shutdown")

if __name__ == "__main__":
    try:
        detection_system = AIDetectionSystem()
        detection_system.run()
    except Exception as e:
        logging.error(f"Failed to start: {e}")
        print(f"Error: {e}")
