# Calibration for FASER Preshower Detector

This repository contains two calibration methods implemented for the new preshower detector of FASER: the **averaging method** and the **fitting method**. These methods allow for independent calibration of the detector and can be executed without integration with FASER's TDAQ system.

## Calibration Methods

### 1. **Averaging Method**
   - The averaging method calculates the reconstructed charge with a sophisticated interpolation method.
   - Detailed description and results can be found [here](https://indico.cern.ch/event/1442377/contributions/6070681/attachments/2905453/5096798/Calibration_Rafaella.pdf).

### 2. **Fitting Method**
   - The fitting method uses fitting techniques to recontruct the charge of each pixel.
   - For details and results, refer to the back slides in [this presentation](https://indico.cern.ch/event/1284854/contributions/5951349/attachments/2889433/5064956/Rafaella_Kotitsa_FASER_iworid.pdf).

## Running the Calibration

Each method is contained in a corresponding folder within the `src` directory. The following steps will guide you through running the calibration:

1. **Input Files**:
   - The `input` folder contains two sample ROOT files with data acquired using the rolling mask pattern.
   - Ensure the `input` folder is placed in the same directory as the method you intend to run.

2. **Build and Run**:
   - Navigate to the directory of the method you wish to run.
   - Use the provided `Makefile` to compile and execute the code:
     ```bash
     make && ./main
     ```
   
3. **Output**:
   - Each calibration run will generate a `calibration.root` file containing the uncalibrated results and a `calibrated.root`, contaning the calibrated results.

## Notes

- These calibration methods can run independently of the FASER TDAQ system.
- The calibration results are stored in a `calibrated.root` file, but the output can be adapted to meet the needs of the TDAQ system or any other required format.

## General Calibration Process

The **general_calibration.pdf** file outlines the full process for calibrating each ASIC in the preshower detector. It includes:

- Instructions for performing threshold and noise scans.
- Detailed guidance on data acquisition and charge calibration.
- Recommended working points based on tests conducted on the ASIC.
- Descriptions of the sampling technique for test pulses used in the calibration process.

## Data Acquisition

The `calibration.cs` and `calibration_changetp.cs` file are C# scripts designed for acquiring the full calibration dataset for the FASER ASIC. They are the same, but `calibration_changetp.cs` acquires the data in different order making the acquisition much faster.They are intended for use with the UNIGE GUI and includes all necessary features for integration with the FASER TDAQ system. The files acquired have the name: Run_superpixel_testpulse_patternStep.daq or .root after conversion.

## Contact

For any questions or comments, please reach out to:
**Rafaella Eleni Kotitsa**  
📧 [rafaella.eleni.kotitsa@cern.ch](mailto:rafaella.eleni.kotitsa@cern.ch)
