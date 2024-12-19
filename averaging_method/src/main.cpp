//Fitting method for the charge calibration of the new preshower detector of FASER.
//Any questions or comments please contact: Rafaella Eleni Kotitsa (rafaella.eleni.kotitsa@cern.ch)
#include <TFile.h>
#include <TH1I.h>
#include <TSystem.h>
#include <TTree.h>
#include <TCollection.h>
#include <TKey.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include <TApplication.h>
#include <TStyle.h>
#include <TH1.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <iostream>
#include <map>
#include <unordered_set>
#include <sstream>
#include <filesystem>
#include <numeric>

// The rolling mask pixel pattern for matching the valid pixels in each event.
std::unordered_set<Int_t> pattern{0, 7, 8, 15, 18, 21, 26, 29, 33, 36, 43, 46, 51, 54, 57, 60};

// Struct to store event data
struct Data {
    bool corrupted; // Flag to check if data is corrupted
    Int_t hit_num; // Number of hits in the event
    std::vector<Int_t>* super_column_id = 0;
    std::vector<Int_t>* super_pixel_id = 0;
    std::vector<Int_t>* pixel_id = 0;
    std::vector<Int_t>* pixel_column = 0;
    std::vector<Int_t>* pixel_row = 0;
    std::vector<Int_t>* charge = 0; // Charge value (in ADC) for each pixel
};

// Struct to define pixel position in the ASIC
struct PixelPosition {
    Int_t super_column_id;
    Int_t super_pixel_id;
    Int_t pixel_id;
    Int_t pixel_column;
    Int_t pixel_row;

    // Equality operator for comparing pixel positions
    bool operator==(const PixelPosition &other) const {
        return (
                super_column_id == other.super_column_id &&
                super_pixel_id == other.super_pixel_id &&
                pixel_id == other.pixel_id
        );
    }

    // Inequality operator for PixelPosition
    bool operator!=(const PixelPosition &other) const {
        return !(*this == other);
    }
};

// Custom hash function for PixelPosition to be used in unordered containers
template <>
struct std::hash<PixelPosition> {
    std::size_t operator()(const PixelPosition& k) const {
        return k.super_column_id * 1000000 + k.super_pixel_id * 10000  + k.pixel_id;
    }
};

// Struct to store charge information for each pixel
struct ChargeInfo {
    Int_t total; // Total charge accumulated across all events
    Int_t samples; // Number of samples (how many events per 1 testpulse injection), to create the average ADC per pixel for the calibration
};

int total_events = 0;
int total_corrupted = 0;

// Function to check if a pixel matches the defined pattern
bool pattern_matches(Int_t pixel_id, Int_t pattern_step) {
    for (auto candidate : pattern) {
        if (((candidate + pattern_step * 16) % 256) == pixel_id) {
            return true;
        }
    }
    return false;
}

using PixelChargeMap = std::unordered_map<PixelPosition, ChargeInfo>;

// Function to read and process actual event data from a ROOT file
PixelChargeMap read_actual(const std::string& file_path, Int_t target_super_pixel_id, Int_t pattern_step = 0) {
    TFile file(file_path.c_str());
    TTree* tree = (TTree *)file.Get("EventData");

    auto event_number = tree->GetEntries();

    Data data;

    // Setting branch addresses to load event data into the "data" struct
    tree->SetBranchAddress("bad_trig", &data.corrupted);
    tree->SetBranchAddress("nhits", &data.hit_num);
    tree->SetBranchAddress("SC_id", &data.super_column_id);
    tree->SetBranchAddress("SP_id", &data.super_pixel_id);
    tree->SetBranchAddress("PIX_id", &data.pixel_id);
    tree->SetBranchAddress("PIX_col", &data.pixel_column);
    tree->SetBranchAddress("PIX_row", &data.pixel_row);
    tree->SetBranchAddress("PIX_adc", &data.charge);

    PixelChargeMap charges;

    // Loop over all events in the tree
    for (Long64_t event = 0; event < event_number; ++event) {
        tree->GetEntry(event); // Load event data into the "data" struct
        total_events += 1; // Increment total event counter

        if (data.corrupted) {
            total_corrupted += 1; // Count corrupted events and skip them
            continue;
        }

        // Loop over all hits (pixels) in the event
        for (std::size_t i = 0; i < data.charge->size(); ++i) {
            auto charge = (*data.charge)[i]; // Get the ADC charge value for the pixel
            auto super_pixel_id = std::abs((*data.super_pixel_id)[i] - 7); // Superpixel ID offset, to match the superpixel orientation of the final ASIC.
            auto pixel_id = (*data.pixel_id)[i]; // Pixel ID

            // Adjust pixel row and column values based on pixel_id
            {
                auto offset_in_row = pixel_id % 16;
                auto row = std::abs((pixel_id / 16) - 15);
                pixel_id = row * 16 + offset_in_row;
            }

            auto pixel_column = (*data.pixel_column)[i];
            auto pixel_row = (*data.pixel_row)[i];

            // Only process pixels that match the desired superpixel and pattern
            if (super_pixel_id != target_super_pixel_id) {
                continue;
            }

            if (!pattern_matches(pixel_id, pattern_step)) {
                continue;
            }

            PixelPosition key{ (*data.super_column_id)[i], super_pixel_id, pixel_id, pixel_column, pixel_row };

            auto& chargeInfo = charges[key]; // Access or create a ChargeInfo entry for the pixel
            chargeInfo.total += charge; // Accumulate charge
            chargeInfo.samples++; // Increment sample count
        }
    }

    tree->ResetBranchAddresses();
    return charges; // Return the map of charges for all pixels in the event
}

// Testpulse values for different runs, here you add the values of injected testpulses you acquired. Uncomment below if you did the full range.
/*
std::vector<int> biases{
    110, 105, 100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 32, 28, 25, 22, 20, 18, 16, 15, 14, 12, 10, 6, 5, 4, 3, 2, 1
};
*/

//only testpulse value to do begin with the uploaded files on gitlab.
std::vector<int> biases{
    15
};

// Predefined map of injected testpulse to charge values (QTP), mapping testpulse to charge in fC.
std::unordered_map<Int_t, Double_t> qtps{
        {1.00, 0.90}, {2.00, 1.44}, {3.00, 1.80}, {4.00, 2.34}, {5.00, 3.24}, {6.00, 4.5},
        {8.00, 5.4}, {9.00, 6.3}, {10.00, 6.3}, {12.00, 7.74}, {14.00, 9.54}, {15.00, 9.9},
        {16.00, 10.8}, {18.00, 12.6}, {20.00, 13.5}, {22.00, 14.4}, {25.00, 16.2}, {28.00, 18.00},
        {32.00, 20.7}, {40.00, 26.1}, {45.00, 28.8}, {50.00, 32.4}, {55.00, 35.1}, {60.00, 37.8},
        {65.00, 41.4}, {70.00, 44.1}, {75.00, 46.80}, {80.00, 50.40}, {85.00, 53.1}, {90.00, 55.80},
        {95.00, 57.60}, {100.00, 61.20}, {105.00, 63.00}, {110.00, 64}
};

// Structure to store exported data for writing to ROOT files, including pixel metadata and charge info
struct ExportedData {
    Int_t super_column_id; // Supercolumn ID
    Int_t super_pixel_id; // Superpixel ID
    Int_t pixel_id; // Pixel ID
    Double_t inverted_qtp; // Inverted QTP value for charge calibration
    Float_t charge; // ADC charge value
    Int_t testpulse; // Testpulse (bias) value injected
    Double_t qtp; // Corresponding charge in fC for the testpulse
    Int_t pixel_column; // Pixel's column position in the detector
    Int_t pixel_row; // Pixel's row position in the detector
};

// Struct to hold charge at a particular bias value for each pixel
struct ChargeAtBias {
    Float_t charge; // Charge value (ADC) measured at this bias
    Int_t bias; // Testpulse/bias value for this charge measurement
};

// Map of pixel positions to charge values at different biases
using BiasedChargeMap = std::unordered_map<PixelPosition, std::vector<ChargeAtBias>>;

// Function to read files, extract charge information, and store it in a map for calibration
std::tuple<BiasedChargeMap, TTree*, TFile*> read_files(std::string folder, std::string dbName) {
    std::vector<std::pair<Int_t, PixelChargeMap>> charges;

    // Loop over all pattern steps and biases to process data.
    // Adjust this if you want to calibrate less superpixels, less patterns etc
    // The nominal is for the calibration data for the whole ASIC is: pattern step "0-16", superpixel_id "0-8"
    //In  the below case only 2 pattern steps will run, change to 0-16 for the full ASIC
    for (int pattern_step = 0; pattern_step < 2; ++pattern_step) {
        for (auto bias : biases) {
            //for the test in gitlab you can run it with only one superpixel, else if you have full data, add 0-8
            for (int super_pixel_id = 0; super_pixel_id < 1; ++super_pixel_id) {
                std::stringstream path;
                path << "./" << folder << "/Run_" << super_pixel_id << "_" << bias << "_" << pattern_step << ".root";

                //std::cout << "Reading :: " << path.str() << '\n'; 

                auto results = read_actual(path.str(), super_pixel_id, pattern_step); // Read actual charge data from file

                //std::cout << "Size :: " << results.size() << '\n';

                charges.push_back(std::pair(bias, std::move(results))); // Store the charge data for each bias
            }
        }
    }

    auto fileName = dbName + ".root";
    auto file = new TFile(fileName.c_str(), "recreate");
    auto data = new TTree("EventData", " EventData");

    ExportedData exported{};

    // Define branches for the output tree
    data->Branch("super_column_id", &exported.super_column_id); 
    data->Branch("super_pixel_id", &exported.super_pixel_id); 
    data->Branch("pixel_id", &exported.pixel_id); 
    data->Branch("charge", &exported.charge);
    data->Branch("testpulse", &exported.testpulse);
    data->Branch("qtp", &exported.qtp);
    data->Branch("pixel_column", &exported.pixel_column);
    data->Branch("pixel_row", &exported.pixel_row);

    std::unordered_map<PixelPosition, std::vector<ChargeAtBias>> biasedCharges;

    //std::cout << "here" << '\n'; tora

    // Fill the tree with charge data
    for (auto& bucket : charges) {
        for (auto& [k, v] : bucket.second) {
            exported.super_column_id = k.super_column_id; //supercolumn number
            exported.super_pixel_id = k.super_pixel_id; //superpixel number
            exported.pixel_id = k.pixel_id; //pixel id
            exported.charge = ((Float_t)v.total) / v.samples; ////average original raw ADC value acquired from the ASIC
            exported.testpulse = bucket.first; //testpulse injection number
            exported.qtp = qtps[bucket.first]; //corresponding charge of the injected testpulse
            exported.pixel_column = k.pixel_column; //pixel column
            exported.pixel_row = k.pixel_row; //pixel row

            auto& line = biasedCharges[k]; // Access or create a vector of ChargeAtBias for this pixel
            line.push_back(ChargeAtBias{exported.charge, bucket.first}); // Add the charge and bias to the vector

            data->Fill();
        }
    }

    data->Write();
    return std::tuple{biasedCharges, data, file};  // Return the map of biased charges, the TTree, and the TFile
}


// Structure to hold the final results for each pixel after calibration
struct Result {
    Double_t original_charge; // Original charge value (from QTP), the injected one from the testpulse
    Double_t calibrated_charge; // Calibrated charge value 
};

// Function to write calibrated data to a ROOT file, storing results for each pixel
std::tuple<TTree*, TFile*> write_calibrated(const std::string& dbName, const std::unordered_map<PixelPosition, std::vector<Result>>& charges) {
    auto fileName = dbName + ".root";
    auto file = new TFile(fileName.c_str(), "recreate");
    auto data = new TTree("EventData"," EventData");

    ExportedData exported{}; // Struct to store exported data

    // Define branches for the output tree (what you need for the calibration results)
    data->Branch("super_column_id", &exported.super_column_id); // supercolumn number
    data->Branch("super_pixel_id", &exported.super_pixel_id); // superpixel number
    data->Branch("pixel_id", &exported.pixel_id); // pixel id
    data->Branch("inverted_qtp", &exported.inverted_qtp);  // reconstructed charge in fC (official calibration goal)
    data->Branch("charge", &exported.charge); // no need to use currently, it will be updated 
    data->Branch("testpulse", &exported.testpulse); // Injected testpule
    data->Branch("qtp", &exported.qtp); // Corresponding injected charge for each testpulse (from the charge-testpulse map)
    data->Branch("pixel_column", &exported.pixel_column); //pixel column
    data->Branch("pixel_row", &exported.pixel_row); //pixel row
    
    // Loop through all pixels and their calibration results
    for (auto& [k, bucket] : charges) {
        for (auto result : bucket) {
            exported.super_column_id = k.super_column_id;
            exported.super_pixel_id = k.super_pixel_id;
            exported.pixel_id = k.pixel_id;
            exported.inverted_qtp = result.calibrated_charge;  // Calibrated charge (fC)
            exported.charge = std::round(result.calibrated_charge * 10) / 10.0f;
            exported.testpulse = 0.0; // Testpulse value (not used in final output)
            exported.qtp = result.original_charge; // Original charge value (the charge value injected from the testpulse)
            exported.pixel_column = k.pixel_column;
            exported.pixel_row = k.pixel_row;

            data->Fill();
        }
    }

    data->Write();
    return std::pair{data, file};
}

// Function to find the calibrated charge for a given digitized ADC value
Double_t calibrated_charge_at(const std::vector<Double_t>& header, const std::vector<Float_t>& pixel, Int_t digitized) {

    if (digitized > pixel[0]) {  // If the ADC value is above the highest recorded value, return max charge (64.0)
        return 64.0;
    }
    if (digitized < pixel[pixel.size() - 1]) {  // If the ADC value is below the lowest recorded value, return 0 charge
        return 0.0;
    }

    // Find the matching range for the digitized value in the pixel charge data
    int from = -1, to = -1;
    int i = 0;
    for (auto x : pixel) {
        if (static_cast<int>(x) == digitized || static_cast<int>(x + 0.99) == digitized) {
            to = i;
            if (from == -1) {
                from = i;
            }
        }
        ++i;
    }

     // Return the average charge corresponding to the digitized ADC value
    return (header[from] + header[to]) / 2;
}

// Main function to run the calibration process
int main() {
    // Read biased charges from input data files, and store the results in the ROOT file
    auto [biasedCharges, data, file] = read_files("input", "calibration");
    delete file;

   //std::cout << "Total Corrupted: " << total_corrupted << " for " << total_events << " events\n"; tora
    std::unordered_map<PixelPosition, std::vector<Result>> results;

    // Loop over all biased charges for calibration
    for (auto& [position, charges] : biasedCharges) {

        std::vector<Double_t > header; // Store original charges (QTP)
        std::vector<Float_t> pixel; // Store corresponding pixel ADC values

        // Loop over all charge values for this pixel
        for (auto charge : charges) {
            auto original_charge = qtps[charge.bias]; // Get original charge value from the QTP map (testpulse and charge injected)
            header.emplace_back(original_charge); // Store the original charge
            pixel.emplace_back(charge.charge); // Store the corresponding ADC value
        }

        // Perform the calibration for each charge
        for (auto& x : charges) {
            auto original_charge = qtps[x.bias];  // Original charge value
            auto calibrated_charge = calibrated_charge_at(header, pixel, (Int_t) x.charge); // Calibrate the charge

            auto& bucket = results[position]; // Store the calibrated result
            bucket.push_back(Result { original_charge, calibrated_charge });
        }
    }

    auto [resultsTree, resultsFile] = write_calibrated("calibrated", results);
    delete resultsFile;

    return 0;
}