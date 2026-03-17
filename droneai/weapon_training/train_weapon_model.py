#!/usr/bin/env python3
"""
Custom Weapon Detection Model Training
Uses the downloaded Kaggle guns-knives dataset
"""

import os
import sys
import shutil
import yaml
from pathlib import Path
from ultralytics import YOLO
import logging

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_corrected_dataset_config():
    """Create corrected dataset configuration for local training"""
    logger.info("Creating corrected dataset configuration...")
    
    # Find the actual dataset path
    base_dir = Path(".")
    yolo_dataset_path = base_dir / "dataset" / "guns-knives-yolo" / "guns-knives-yolo"
    
    if not yolo_dataset_path.exists():
        logger.error(f"Dataset not found at {yolo_dataset_path}")
        return None
    
    # Create corrected dataset configuration
    dataset_config = {
        'path': str(yolo_dataset_path.absolute()),
        'train': 'train/images',
        'val': 'valid/images',
        'names': {
            0: 'knife',
            1: 'pistol'
        },
        'nc': 2
    }
    
    # Save corrected configuration
    config_path = yolo_dataset_path / "corrected_data.yaml"
    with open(config_path, 'w') as f:
        yaml.dump(dataset_config, f, default_flow_style=False)
    
    logger.info(f"Dataset configuration saved to {config_path}")
    
    # Verify dataset structure
    train_images = yolo_dataset_path / "train" / "images"
    train_labels = yolo_dataset_path / "train" / "labels"
    val_images = yolo_dataset_path / "valid" / "images" 
    val_labels = yolo_dataset_path / "valid" / "labels"
    
    logger.info("Dataset structure verification:")
    logger.info(f"Train images: {len(list(train_images.glob('*.jpg')))} files")
    logger.info(f"Train labels: {len(list(train_labels.glob('*.txt')))} files")
    logger.info(f"Val images: {len(list(val_images.glob('*.jpg')))} files")
    logger.info(f"Val labels: {len(list(val_labels.glob('*.txt')))} files")
    
    return config_path

def train_custom_weapon_model(dataset_config_path, epochs=50):
    """Train the custom weapon detection model"""
    logger.info(f"Starting weapon detection model training for {epochs} epochs...")
    
    try:
        # Initialize YOLO model with pre-trained weights
        model = YOLO('yolov8n.pt')  # Nano model for faster training
        
        # Set up results directory
        results_dir = Path("results")
        results_dir.mkdir(exist_ok=True)
        
        logger.info("Training configuration:")
        logger.info(f"- Dataset: {dataset_config_path}")
        logger.info(f"- Epochs: {epochs}")
        logger.info(f"- Model: YOLOv8n")
        logger.info(f"- Device: CPU (training may be slow)")
        logger.info(f"- Batch size: 8 (reduced for CPU)")
        
        # Train the model
        results = model.train(
            data=str(dataset_config_path),
            epochs=epochs,
            imgsz=640,
            batch=8,  # Smaller batch for CPU training
            name='weapon_detection',
            project=str(results_dir),
            save=True,
            device='cpu',
            patience=15,  # Early stopping patience
            save_period=5,  # Save checkpoint every 5 epochs
            plots=True,  # Generate training plots
            val=True,    # Run validation
            verbose=True
        )
        
        # Save the best model to models directory
        models_dir = Path("models")
        models_dir.mkdir(exist_ok=True)
        
        best_model_path = models_dir / "weapon_detection_best.pt"
        last_model_path = models_dir / "weapon_detection_last.pt"
        
        # Copy the trained models
        training_results_dir = results_dir / "weapon_detection"
        weights_dir = training_results_dir / "weights"
        
        if (weights_dir / "best.pt").exists():
            shutil.copy(weights_dir / "best.pt", best_model_path)
            logger.info(f"Best model saved to: {best_model_path}")
        
        if (weights_dir / "last.pt").exists():
            shutil.copy(weights_dir / "last.pt", last_model_path)
            logger.info(f"Last model saved to: {last_model_path}")
        
        logger.info("Training completed successfully!")
        logger.info(f"Training results saved to: {training_results_dir}")
        
        return best_model_path
        
    except Exception as e:
        logger.error(f"Error during training: {e}")
        return None

def test_trained_model(model_path):
    """Test the trained weapon detection model"""
    logger.info("Testing trained model...")
    
    try:
        # Load the trained model
        model = YOLO(str(model_path))
        
        # Get model info
        logger.info(f"Model classes: {model.names}")
        logger.info(f"Number of classes: {len(model.names)}")
        
        # Run validation on test set
        logger.info("Running validation...")
        val_results = model.val()
        
        # Print validation results
        if hasattr(val_results, 'box'):
            logger.info(f"Validation Results:")
            logger.info(f"- mAP50: {val_results.box.map50:.4f}")
            logger.info(f"- mAP50-95: {val_results.box.map:.4f}")
            
            # Per-class results
            if hasattr(val_results.box, 'maps'):
                for i, class_map in enumerate(val_results.box.maps):
                    class_name = model.names[i]
                    logger.info(f"- {class_name} mAP50: {class_map:.4f}")
        
        # Test on some sample images
        dataset_path = Path("dataset/guns-knives-yolo/guns-knives-yolo")
        test_images_dir = dataset_path / "test" / "images"
        
        if test_images_dir.exists():
            test_images = list(test_images_dir.glob("*.jpg"))[:5]
            logger.info(f"Testing on {len(test_images)} sample images...")
            
            for img_path in test_images:
                results = model(str(img_path))
                detections = results[0].boxes
                if detections is not None and len(detections) > 0:
                    logger.info(f"- {img_path.name}: {len(detections)} objects detected")
                    for box in detections:
                        class_id = int(box.cls[0])
                        confidence = float(box.conf[0])
                        class_name = model.names[class_id]
                        logger.info(f"  - {class_name}: {confidence:.3f}")
                else:
                    logger.info(f"- {img_path.name}: No objects detected")
        
        return True
        
    except Exception as e:
        logger.error(f"Error testing model: {e}")
        return False

def main():
    """Main training function"""
    print("🔫 Custom Weapon Detection Model Training")
    print("=========================================")
    print()
    
    # Check if dataset exists
    dataset_path = Path("dataset/guns-knives-yolo/guns-knives-yolo")
    if not dataset_path.exists():
        print("❌ Dataset not found!")
        print("Please run the following command first:")
        print("python weapon_training_pipeline.py")
        return
    
    # Get training epochs from command line
    epochs = 50
    if len(sys.argv) > 1:
        try:
            epochs = int(sys.argv[1])
        except ValueError:
            print("Invalid epochs value. Using default: 50")
    
    print(f"🚀 Starting training for {epochs} epochs...")
    print("⏳ This may take a while on CPU...")
    print()
    
    # Step 1: Create corrected dataset configuration
    dataset_config_path = create_corrected_dataset_config()
    if not dataset_config_path:
        print("❌ Failed to create dataset configuration")
        return
    
    # Step 2: Train the model
    model_path = train_custom_weapon_model(dataset_config_path, epochs)
    if not model_path:
        print("❌ Training failed")
        return
    
    # Step 3: Test the model
    if not test_trained_model(model_path):
        print("❌ Model testing failed")
        return
    
    print()
    print("🎉 SUCCESS! Custom weapon detection model trained!")
    print("=" * 50)
    print(f"📍 Model location: {model_path}")
    print(f"📊 Training results: results/weapon_detection/")
    print()
    print("📋 Next steps:")
    print("1. Integrate the model into your detection system")
    print("2. Test with real firearm images")
    print("3. Fine-tune if needed")
    print()
    print("🔧 To integrate into your detection system:")
    print(f"   - Replace 'yolov8n.pt' with '{model_path}' in weapon_detector.py")

if __name__ == "__main__":
    main()
