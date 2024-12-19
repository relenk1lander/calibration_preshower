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

// The rolling mask pixel pattern for matching
std::unordered_set<Int_t> pattern{0, 7, 8, 15, 18, 21, 26, 29, 33, 36, 43, 46, 51, 54, 57, 60};

// Struct to store event data
struct Data {
    bool corrupted; // Flag to check if data is corrupted
    Int_t hit_num;  // Number of hits in the event
    std::vector<Int_t>* super_column_id = 0;
    std::vector<Int_t>* super_pixel_id = 0;
    std::vector<Int_t>* pixel_id = 0;
    std::vector<Int_t>* pixel_column = 0;
    std::vector<Int_t>* pixel_row = 0;
    std::vector<Int_t>* charge = 0;
};

// Struct to define pixel position
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

    // Inequality operator
    bool operator!=(const PixelPosition &other) const {
        return !(*this == other);
    }
};

// Custom hash function for PixelPosition
template <>
struct std::hash<PixelPosition> {
    std::size_t operator()(const PixelPosition& k) const {
        return k.super_column_id * 1000000 + k.super_pixel_id * 10000  + k.pixel_id;
    }
};

// Struct to store charge information
struct ChargeInfo {
    Int_t total;  // Total charge
    Int_t samples; // Number of samples (how many events per 1 testpulse injection)
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

// Function to read the actual event data from a ROOT file
PixelChargeMap read_actual(const std::string& file_path, Int_t target_super_pixel_id, Int_t pattern_step = 0) {
    TFile file(file_path.c_str());
    TTree* tree = (TTree *)file.Get("EventData");

    auto event_number = tree->GetEntries();

    Data data;

    // Setting branch addresses
    tree->SetBranchAddress("bad_trig", &data.corrupted);
    tree->SetBranchAddress("nhits", &data.hit_num);
    tree->SetBranchAddress("SC_id", &data.super_column_id);
    tree->SetBranchAddress("SP_id", &data.super_pixel_id);
    tree->SetBranchAddress("PIX_id", &data.pixel_id);
    tree->SetBranchAddress("PIX_col", &data.pixel_column);
    tree->SetBranchAddress("PIX_row", &data.pixel_row);
    tree->SetBranchAddress("PIX_adc", &data.charge);

    PixelChargeMap charges;

    for (Long64_t event = 0; event < event_number; ++event) {
        tree->GetEntry(event);
        total_events += 1;

        if (data.corrupted) {
            total_corrupted += 1;
            continue; // Skip corrupted events
        }

        for (std::size_t i = 0; i < data.charge->size(); ++i) {
            auto charge = (*data.charge)[i];
            auto super_pixel_id = std::abs((*data.super_pixel_id)[i] - 7);
            auto pixel_id = (*data.pixel_id)[i];

            // Adjust pixel row and column values based on pixel_id
            {
                auto offset_in_row = pixel_id % 16;
                auto row = std::abs((pixel_id / 16) - 15);
                pixel_id = row * 16 + offset_in_row;
            }

            auto pixel_column = (*data.pixel_column)[i];
            auto pixel_row = (*data.pixel_row)[i];

            if (super_pixel_id != target_super_pixel_id) {
                continue;
            }

            if (!pattern_matches(pixel_id, pattern_step)) {
                continue;
            }

            PixelPosition key{ (*data.super_column_id)[i], super_pixel_id, pixel_id, pixel_column, pixel_row };

            auto& chargeInfo = charges[key];
            chargeInfo.total += charge;
            chargeInfo.samples++;
        }
    }

    tree->ResetBranchAddresses();
    return charges;
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

// Predefined charge values corresponding to testpulse (Map of how much charge corresponds to each injected testpulse value)
std::unordered_map<Int_t, Double_t> qtps{
        {1.00, 0.90}, {2.00, 1.44}, {3.00, 1.80}, {4.00, 2.34}, {5.00, 3.24}, {6.00, 4.5},
        {8.00, 5.4}, {9.00, 6.3}, {10.00, 6.3}, {12.00, 7.74}, {14.00, 9.54}, {15.00, 9.9},
        {16.00, 10.8}, {18.00, 12.6}, {20.00, 13.5}, {22.00, 14.4}, {25.00, 16.2}, {28.00, 18.00},
        {32.00, 20.7}, {40.00, 26.1}, {45.00, 28.8}, {50.00, 32.4}, {55.00, 35.1}, {60.00, 37.8},
        {65.00, 41.4}, {70.00, 44.1}, {75.00, 46.80}, {80.00, 50.40}, {85.00, 53.1}, {90.00, 55.80},
        {95.00, 57.60}, {100.00, 61.20}, {105.00, 63.00}, {110.00, 64}
};

// Structure to store exported data for writing to ROOT files
struct ExportedData {
    Int_t super_column_id;
    Int_t super_pixel_id;
    Int_t pixel_id;
    Double_t inverted_qtp;
    Float_t charge;
    Int_t testpulse;
    Double_t qtp;
    Int_t pixel_column;
    Int_t pixel_row;
};

// Struct to hold charge at a particular bias value
struct ChargeAtBias {
    Float_t charge;
    Int_t bias;
};

using BiasedChargeMap = std::unordered_map<PixelPosition, std::vector<ChargeAtBias>>;

// Function to read files, extract charge information, and store it in a map
std::tuple<BiasedChargeMap, TTree*, TFile*> read_files(std::string folder, std::string dbName) {
    std::vector<std::pair<Int_t, PixelChargeMap>> charges;

    // Loop over all pattern steps and biases to process data.
    // Adjust this if you want to calibrate less superpixels, less patterns etc
    // The nominal is for the calibration data for the whole ASIC is: pattern step "0-16", superpixel_id "0-8"
    //Below only 2 pattern steps will be calibrated, change to the 0-16 for the full ASIC
    for (int pattern_step = 0; pattern_step < 2; ++pattern_step) {
        for (auto bias : biases) {
            //for the test in gitlab you can run it with only one superpixel, else if you have full data, add 0-8
            for (int super_pixel_id = 0; super_pixel_id < 1; ++super_pixel_id) {
                std::stringstream path;
                path << "./" << folder << "/Run_" << super_pixel_id << "_" << bias << "_" << pattern_step << ".root";

                //std::cout << "Reading :: " << path.str() << '\n'; an diavazei ta files tora

                auto results = read_actual(path.str(), super_pixel_id, pattern_step);

                //std::cout << "Size :: " << results.size() << '\n'; an diavazei ta files tora

                charges.push_back(std::pair(bias, std::move(results)));
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

    // Fill the tree with charge data
    for (auto& bucket : charges) {
        for (auto& [k, v] : bucket.second) {
            exported.super_column_id = k.super_column_id; //supercolumn id
            exported.super_pixel_id = k.super_pixel_id; //superpixel id
            exported.pixel_id = k.pixel_id; //pixel id
            exported.charge = ((Float_t)v.total) / v.samples; //average original raw ADC value acquired from the ASIC.
            exported.testpulse = bucket.first; //testpulse injection number
            exported.qtp = qtps[bucket.first]; //corresponding charge of the injected testpulse
            exported.pixel_column = k.pixel_column; //pixel column
            exported.pixel_row = k.pixel_row; //pixel row

            auto& line = biasedCharges[k];
            line.push_back(ChargeAtBias{exported.charge, bucket.first});

            data->Fill();
        }
    }

    data->Write();
    return std::tuple{biasedCharges, data, file};
}

// Structure to hold calibration function parameters
struct CalibrationFunction {
    Double_t a;
    Double_t b;
    Double_t c;
    Double_t chi_square_ndf;

    // Function to apply the calibration function
    Double_t apply(Double_t qtp) {
        return c * std::pow(1 - exp(-a * qtp), b);
    }

    // Inverse function for calibration
    Double_t apply_inverse(Double_t charge) {
        auto result = -std::log(1 - std::pow(charge / c, 1 / b)) / a;
        return std::isnan(result) ? 64.0 : std::min(result, 64.0);
    }
};

// Reference calibration function parameters. This is for the calibrated ADC, not charge
// It is optional to use, since the main goal discussed until now is for the reconstructed charge in fC
// This is just an extra implementattion, to have an idea of how an ideal ADC value of a hit should look like
CalibrationFunction reference_function{0.095, 2.2, 15};

// Function to calculate calibration parameters from the injected charges charges
std::unordered_map<PixelPosition, CalibrationFunction> calcCalibrationParams(const BiasedChargeMap& biasedCharges, TTree* data) {
    std::unordered_map<PixelPosition, CalibrationFunction> calibrationParams;
    
    for (auto& [p, line] : biasedCharges) {
        if (line.size() < 2) {
            continue; // Skip if not enough data points
        }

        auto max_charge = std::max_element(line.begin(), line.end(), [](auto a, auto b) { return a.charge < b.charge; })->charge;

        std::stringstream function_def;
        function_def << max_charge << "*pow((1-exp(-[0]*x)),[1])";

        auto to_string = function_def.str();

        TF1 ex("tf_name", to_string.c_str());
        for (auto& x : line) {
            //std::cout << "> " << x.charge << '\n'; tora
        }
        //std::cout << "Fit with :: " << to_string << ", max = " << max_charge << '\n'; tora
        ex.SetParameters(1./12., 1);

        std::stringstream predicate;
        predicate << "pixel_id == " << p.pixel_id
                  << "&& super_column_id == " << p.super_column_id
                  << "&& super_pixel_id == " << p.super_pixel_id;

        data->Fit("tf_name", "charge:qtp", predicate.str().c_str(), "WQ0N");
       
        // Store calibration parameters
        double chi_square_ndf = ex.GetChisquare() / ex.GetNDF();  
        calibrationParams[p] = { ex.GetParameter(0), ex.GetParameter(1), max_charge + 0.001f, chi_square_ndf };

        //std::cout << "Fit params: " << ex.GetParameter(0) << ", " << ex.GetParameter(1) << ", " << max_charge + 0.001f << " :: chi_sq = " << chi_square_ndf << '\n'; tora
       
       //If you want to see how the fits look like, for checks. Use with caution since it will produce more than 26000 fits.
       /*
        TCanvas c1("c1", "Fit Canvas", 1600, 1200);
        TGraph graph(line.size());
        for (size_t i = 0; i < line.size(); ++i) {
            graph.SetPoint(i, qtps[line[i].bias], line[i].charge);
        }
        graph.SetMarkerStyle(20);
        graph.SetMarkerColor(kBlue);
        graph.SetTitle(Form("Fit for Pixel (SC: %d, SP: %d, P: %d)", p.super_column_id, p.super_pixel_id, p.pixel_id));
        graph.GetXaxis()->SetTitle("Charge(fC)");
        graph.GetYaxis()->SetTitle("ADC value");
        graph.Draw("AP");
      
        ex.SetRange(graph.GetXaxis()->GetXmin(), graph.GetXaxis()->GetXmax());
        //ex.Draw("same");

        TLegend legend(0.1, 0.7, 0.3, 0.9);
        legend.AddEntry(&graph, "Data", "p");
        legend.AddEntry(&ex, Form("Fit: a=%.3f, b=%.3f, c=%.3f", ex.GetParameter(0), ex.GetParameter(1), max_charge + 0.001f), "l");
        legend.AddEntry((TObject*)0, Form("chi2/ndf = %.2f", chi_square_ndf), "");
        legend.Draw();

        c1.Update();
        std::filesystem::create_directory("fits");
        c1.SaveAs(Form("fits/fit_pixel_%d_sc_%d_sp_%d_pc_%d_pr_%d.png", p.pixel_id, p.super_column_id, p.super_pixel_id, p.pixel_column, p.pixel_row));
        */
    }

    return calibrationParams;
}

// Structure to hold the final results for each pixel
struct Result {
    Int_t bias;
    Double_t inverted_qtp;
    Double_t calibrated_charge;
};

std::tuple<TTree*, TFile*> write_calibrated(const std::string& dbName, const std::unordered_map<PixelPosition, std::vector<Result>>& charges) {
    auto fileName = dbName + ".root";
    auto file = new TFile(fileName.c_str(), "recreate");
    auto data = new TTree("EventData"," EventData");

    ExportedData exported{};

    // Define branches for the output tree, what you need for the calibration
    data->Branch("super_column_id", &exported.super_column_id); // supercolumn number
    data->Branch("super_pixel_id", &exported.super_pixel_id); // superpixel number
    data->Branch("pixel_id", &exported.pixel_id); // pixel id
    data->Branch("inverted_qtp", &exported.inverted_qtp); // reconstructed charge in fC (official calibration goal)
    data->Branch("charge", &exported.charge); // reconstructed ADC value (optional to use, no need for TDAQ probably)
    data->Branch("testpulse", &exported.testpulse); // Injected testpule
    data->Branch("qtp", &exported.qtp); // Corresponding injected charge for each testpulse (from the charge-testpulse map)
    data->Branch("pixel_column", &exported.pixel_column); //pixel column
    data->Branch("pixel_row", &exported.pixel_row); //pixel row

    for (auto& [k, bucket] : charges) {
        for (auto result : bucket) {
            exported.super_column_id = k.super_column_id;
            exported.super_pixel_id = k.super_pixel_id;
            exported.pixel_id = k.pixel_id;
            exported.inverted_qtp = result.inverted_qtp;
            exported.charge = std::round(result.calibrated_charge * 10) / 10.0f;
            exported.testpulse = result.bias;
            exported.qtp = qtps[result.bias];
            exported.pixel_column = k.pixel_column;
            exported.pixel_row = k.pixel_row;

            data->Fill();
        }
    }

    data->Write();
    return std::pair{data, file};
}

int main() {

    // Read input data from files and extract biased charge information
    auto [biasedCharges, data, file] = read_files("input", "calibration");
    auto calibrationParams = calcCalibrationParams(biasedCharges, data);
    delete file;

    std::cout << "Total Corrupted: " << total_corrupted << " for " << total_events << " events\n"; tora
    std::unordered_map<PixelPosition, std::vector<Result>> results;
    std::cout << "Total: " << biasedCharges.size() << " events\n"; tora

    // Loop over all biased charges and apply the calibration functions
    for (auto& [position, charges] : biasedCharges) {
        auto function = calibrationParams[position];

        //Adjust, comment or uncomment if you want to also export the data to inport in the offline allpix squared calibration system
        std::cout << '\n' << "Position: SC: " << position.super_column_id << ", SP: " << position.super_pixel_id << ", P: " << position.pixel_id << '\n'; 
        std::cout << "Coordinates: c: " << position.pixel_column << ", r: " << position.pixel_row << '\n'; 

        auto [a, b, c, chi_square_ndf] = function;
        std::cout << "Fit params: " << a << ", " << b << ", " << c << " :: chi_sq = " << chi_square_ndf << '\n'; 
        std::cout << '\n'; //tora

        // Process each charge value for the pixel and apply the inverse calibration
        for (auto& x : charges) {
            auto inverted_qtp = function.apply_inverse(std::round(x.charge));
            auto calibrated_charge = reference_function.apply(inverted_qtp);
            auto& bucket = results[position];
            bucket.push_back(Result{x.bias, inverted_qtp, calibrated_charge});
            std::cout << x.charge << ',';
        }
    }

    // Write the calibrated data to a ROOT file
    auto [resultsTree, resultsFile] = write_calibrated("calibrated", results);
    delete resultsFile;

    return 0;
}
