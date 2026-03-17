import json
import logging
import time
import requests
import threading
from typing import List, Dict, Any, Optional
from datetime import datetime


class LocalLLMAnalyzer:
    """Local LLM scene analyzer using Ollama for threat assessment."""
    
    def __init__(self, config: Dict[str, Any]):
        """Initialize the Local LLM analyzer with configuration."""
        self.config = config
        self.llm_config = config.get('llm', {})
        self.enabled = self.llm_config.get('enabled', False)
        
        if not self.enabled:
            logging.info("Local LLM analyzer disabled in config")
            return
            
        self.ollama_url = self.llm_config.get('ollama_url', 'http://localhost:11434')
        self.model = self.llm_config.get('model', 'llama3.2:latest')
        self.temperature = self.llm_config.get('temperature', 0.3)
        self.max_tokens = self.llm_config.get('max_tokens', 200)
        
        self.last_analysis = None
        self.analysis_lock = threading.Lock()
        
        # Test Ollama connection
        # self._test_connection()  # Temporarily disabled to test weapon detection
        logging.info("LLM analyzer initialized (connection test disabled)")
        
        logging.info(f"Local LLM analyzer initialized with model: {self.model}")
    
    def _test_connection(self):
        """Test connection to Ollama server."""
        try:
            response = requests.get(f"{self.ollama_url}/api/tags", timeout=5)
            if response.status_code == 200:
                models = response.json().get('models', [])
                model_names = [m['name'] for m in models]
                logging.info(f"Connected to Ollama. Available models: {model_names}")
                
                if self.model not in model_names:
                    logging.warning(f"Model {self.model} not found. Available: {model_names}")
                    if model_names:
                        self.model = model_names[0]
                        logging.info(f"Using available model: {self.model}")
            else:
                logging.error(f"Failed to connect to Ollama: HTTP {response.status_code}")
                self.enabled = False
        except Exception as e:
            logging.error(f"Error connecting to Ollama: {e}")
            self.enabled = False
    
    def analyze_scene_async(self, detections: List[Dict], frame_info: Dict):
        """Analyze scene asynchronously to avoid blocking main thread."""
        if not self.enabled:
            return
            
        # Run analysis in background thread
        analysis_thread = threading.Thread(
            target=self._analyze_scene,
            args=(detections, frame_info),
            daemon=True
        )
        analysis_thread.start()
    
    def _analyze_scene(self, detections: List[Dict], frame_info: Dict):
        """Analyze the current scene with detected objects."""
        try:
            # Prepare detection summary
            summary = self._prepare_detection_summary(detections, frame_info)
            
            # Create analysis prompt
            prompt = self._create_analysis_prompt(summary)
            
            # Send to Ollama
            response = self._query_ollama(prompt)
            
            if response:
                # Parse and store analysis
                analysis = self._parse_analysis_response(response, detections)
                
                with self.analysis_lock:
                    self.last_analysis = {
                        'timestamp': datetime.now(),
                        'analysis': analysis,
                        'raw_response': response
                    }
                    
                logging.info(f"Scene analysis completed: {analysis.get('threat_level', 'unknown')}")
            
        except Exception as e:
            logging.error(f"Error in scene analysis: {e}")
    
    def _prepare_detection_summary(self, detections: List[Dict], frame_info: Dict) -> Dict:
        """Prepare a summary of current detections."""
        humans = [d for d in detections if d.get('type') == 'human']
        dangerous = [d for d in detections if d.get('is_dangerous', False)]
        objects = [d for d in detections if not d.get('is_dangerous', False) and d.get('type') != 'human']
        
        return {
            'timestamp': frame_info.get('timestamp', datetime.now().strftime('%H:%M:%S')),
            'fps': frame_info.get('fps', 0),
            'total_detections': len(detections),
            'humans': {
                'count': len(humans),
                'details': [{'class': d['class_name'], 'confidence': d['confidence']} for d in humans]
            },
            'dangerous_objects': {
                'count': len(dangerous),
                'details': [{'class': d['class_name'], 'confidence': d['confidence']} for d in dangerous]
            },
            'other_objects': {
                'count': len(objects),
                'details': [{'class': d['class_name'], 'confidence': d['confidence']} for d in objects]
            }
        }
    
    def _create_analysis_prompt(self, summary: Dict) -> str:
        """Create analysis prompt for the LLM."""
        dangerous_count = summary['dangerous_objects']['count']
        human_count = summary['humans']['count']
        
        prompt = f"""You are a security AI analyzing a surveillance scene. Provide a brief security assessment.

CURRENT SCENE ({summary['timestamp']}):
- Humans detected: {human_count}
- Dangerous objects: {dangerous_count}
- Other objects: {summary['other_objects']['count']}

DANGEROUS OBJECTS DETECTED:
{self._format_detections(summary['dangerous_objects']['details'])}

HUMANS DETECTED:
{self._format_detections(summary['humans']['details'])}

Based on this information, provide:
1. Threat Level: LOW, MEDIUM, HIGH, or CRITICAL
2. Brief Assessment: 1-2 sentences about the situation
3. Recommended Action: What security personnel should do

Respond in JSON format:
{{
    "threat_level": "LOW/MEDIUM/HIGH/CRITICAL",
    "assessment": "Brief description of the situation",
    "recommendation": "Recommended security action"
}}"""
        
        return prompt
    
    def _format_detections(self, detections: List[Dict]) -> str:
        """Format detection details for the prompt."""
        if not detections:
            return "None"
            
        formatted = []
        for detection in detections[:3]:  # Limit to first 3 to keep prompt concise
            formatted.append(f"- {detection['class']} (confidence: {detection['confidence']:.2f})")
        
        if len(detections) > 3:
            formatted.append(f"- ... and {len(detections) - 3} more")
            
        return "\n".join(formatted)
    
    def _query_ollama(self, prompt: str) -> Optional[str]:
        """Send query to Ollama and get response."""
        try:
            payload = {
                "model": self.model,
                "prompt": prompt,
                "stream": False,
                "options": {
                    "temperature": self.temperature,
                    "num_predict": self.max_tokens
                }
            }
            
            response = requests.post(
                f"{self.ollama_url}/api/generate",
                json=payload,
                timeout=30
            )
            
            if response.status_code == 200:
                result = response.json()
                return result.get('response', '').strip()
            else:
                logging.error(f"Ollama API error: HTTP {response.status_code}")
                return None
                
        except Exception as e:
            logging.error(f"Error querying Ollama: {e}")
            return None
    
    def _parse_analysis_response(self, response: str, detections: List[Dict]) -> Dict:
        """Parse the LLM response into structured analysis."""
        try:
            # Try to extract JSON from response
            start_idx = response.find('{')
            end_idx = response.rfind('}') + 1
            
            if start_idx >= 0 and end_idx > start_idx:
                json_str = response[start_idx:end_idx]
                parsed = json.loads(json_str)
                
                # Validate required fields
                if all(key in parsed for key in ['threat_level', 'assessment', 'recommendation']):
                    return {
                        'threat_level': parsed['threat_level'].upper(),
                        'assessment': parsed['assessment'][:200],  # Limit length
                        'recommendation': parsed['recommendation'][:200],
                        'dangerous_count': len([d for d in detections if d.get('is_dangerous', False)]),
                        'human_count': len([d for d in detections if d.get('type') == 'human'])
                    }
            
        except (json.JSONDecodeError, KeyError) as e:
            logging.warning(f"Failed to parse LLM response as JSON: {e}")
        
        # Fallback analysis based on detections
        return self._create_fallback_analysis(detections, response)
    
    def _create_fallback_analysis(self, detections: List[Dict], raw_response: str) -> Dict:
        """Create fallback analysis if LLM response can't be parsed."""
        dangerous_count = len([d for d in detections if d.get('is_dangerous', False)])
        human_count = len([d for d in detections if d.get('type') == 'human'])
        
        if dangerous_count > 0:
            threat_level = "HIGH" if dangerous_count > 1 else "MEDIUM"
            assessment = f"Detected {dangerous_count} dangerous object(s) with {human_count} person(s) in scene"
            recommendation = "Immediate attention required - verify weapons and assess threat"
        elif human_count > 3:
            threat_level = "MEDIUM"
            assessment = f"Multiple persons ({human_count}) detected in secured area"
            recommendation = "Monitor situation and verify authorized personnel"
        else:
            threat_level = "LOW"
            assessment = "Normal activity detected"
            recommendation = "Continue monitoring"
        
        return {
            'threat_level': threat_level,
            'assessment': assessment,
            'recommendation': recommendation,
            'dangerous_count': dangerous_count,
            'human_count': human_count,
            'fallback': True,
            'raw_response': raw_response[:100] if raw_response else None
        }
    
    def get_last_analysis(self) -> Optional[Dict]:
        """Get the most recent scene analysis."""
        with self.analysis_lock:
            return self.last_analysis.copy() if self.last_analysis else None
    
    def stop_analyzer(self):
        """Stop the analyzer (cleanup method for compatibility)."""
        logging.info("Local LLM analyzer stopped")
        self.enabled = False
    
    def is_enabled(self) -> bool:
        """Check if the analyzer is enabled and functional."""
        return self.enabled
