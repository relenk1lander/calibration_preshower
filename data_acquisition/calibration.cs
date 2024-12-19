////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//contact rafaella.eleni.kotitsa@cern.ch for questions or comments
// This script defines the masked pixels starting from a list of noisy pixels to mask. Syntax for the noisy pixels: {SC_id,PIX_col,PIX_row}
//static byte[] TEST_PULSE_BIAS = { 255 };
//add the testpulse values you want to acquire
static byte[] TEST_PULSE_BIAS = {255, 240, 230, 220, 210, 200, 195, 190, 185, 180, 175, 170, 165, 160, 155, 150, 145, 140, 135, 130, 125, 120, 115, 110, 105, 100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 32, 28, 25, 22, 20, 18, 16, 15, 14, 12, 10, 9, 8, 6, 5, 4, 3, 2, 1};

string[,,] mask_map = new string[13, 8, 16];

void ScriptMain()
{
	// A
	int chip_address 		= 0;
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.CHIP_ADDRESS", chip_address);

	int sel 				= 0;
	int reset_sel 			= 0;
	bool testpulse_nreset 	= false;
	int reset_start			= 0;

        // Creating masking map, default map is all unmasked
     // Masking map (1 = masked, 0 = unmasked), counting from bottom left pixel 
    for(int super_column_nb=0;super_column_nb<13;super_column_nb++){
        for(int SP_nb=0; SP_nb<8; SP_nb++){
            for(int row_nb=0; row_nb<16; row_nb++){
                mask_map[super_column_nb, SP_nb, row_nb] = "0000000000000000";
                //mask_map[super_column_nb, SP_nb, row_nb] = "1111111111111111";
            }
        }
    }
    
    // --------------Reading mask file
    String line;
    int temp_SC;
    int temp_SP;
    int temp_row;
    int temp_col;
    StreamReader sr = new StreamReader("./Mask_Pattern.txt");
    //Read the first line of text
    line = sr.ReadLine();
    Console.WriteLine("Masking pixels:");
    while (line != null)
    {
        // Assigning mask status to the corresponding pixel
        line = line.Substring(1,line.Length-2);
        //line = line.Remove(0);
        string[] subline = line.Split(',');
        temp_SC = Int32.Parse(subline[0]);
        temp_SP = Int32.Parse(subline[1]);
        temp_row = 15-Int32.Parse(subline[2]);
        temp_col = Int32.Parse(subline[3]);
        Console.WriteLine($"SC: {subline[0]}, SP: {subline[1]}, row: {subline[2]}, col: {subline[3]}");
        mask_map[temp_SC,temp_SP,temp_row] = mask_map[temp_SC,temp_SP,temp_row].Remove(temp_col, 1).Insert(temp_col, "1");
        Console.WriteLine(mask_map[temp_SC,temp_SP,temp_row]);
        //Read the next line
        line = sr.ReadLine();
    }

	ApplyMask();

	// ConfigureSPI(12); // B

	// SendMask();

	// BeforeMask();

	// MaskAll();

	// applyPattern(0, 0, 0);

	// AfterMask();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// void setBiasTestpulse(int value) {
//     //Sends a single SPI command
//     int bias_testpulse_cmd 	= 12;	//0b01100;
// 	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", bias_testpulse_cmd);
// 	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", value);
// }

void ApplyMask() {
    for(int superPixelId = 0; superPixelId < 1; ++superPixelId) {
		foreach (byte bias in TEST_PULSE_BIAS) {
			ConfigureSPI(bias);
			for (int j = 0; j < 2; ++j) {
				BeforeMask();
				MaskAll();
			    for (int superColumnId = 0; superColumnId < 1; ++superColumnId) {
					applyPattern(superColumnId, superPixelId, j);
					super_column_configure();
        			update_param_and_start_spi();
				}
				AfterMask();
				Console.WriteLine($"Testing SP {superPixelId}, Bias{bias}, Pattern {j}");
				StartReadout($"/home/dpnc-lab/Desktop/rafi/Run_{superPixelId}_{bias}_{j}", true, false, false);
				Sync.Sleep(500);
				StopReadout();
				string daq_size = BoardLib.XferKBytes;
				Console.WriteLine($"DAQ size is {daq_size}");				 
				Sync.Sleep(1000);
				//break;
			}
		}

	}
}

public void applyPattern(int superColumnId, int superPixelId, int fromRow) {
	setMaskSuperPixel(superColumnId, superPixelId);
	
	//normal pattern
	ulong pattern = 0b1110110110110111101101111110110111011011110110110111111001111110;
	//test pattern
	//ulong pattern = 0b1111111111111111111111111111111111111111111111111111111111111111;
	//raf_patterntohex = EDB7 B7ED DBDB 7E7E
	//ulong pattern = 0b1110110110110111 1011011111101101 1101101111011011 0111111001111110;
	//hex from gui = 1000100010001000 1000100010001000 1000100000010001 00000001

	for (int i = 0; i < 4; ++i) {
        ulong mask = (pattern >> (i * 16) & 0b1111111111111111);

		Console.WriteLine($"mask section {(int)mask}");

		int row = (fromRow + i) % 16;

        int deadMask = Convert.ToInt32(mask_map[superColumnId, superPixelId, row], 2);

		Console.WriteLine($"dead mask {deadMask}");

        setRowMask(superColumnId, superPixelId, row, ((int)mask) | deadMask);
	}
}

public void MaskAll() {
	for (int superColumnId = 0; superColumnId < 13; ++superColumnId) {
		for(int superPixelId = 0; superPixelId < 8; ++superPixelId) {
			setMaskSuperPixel(superColumnId, superPixelId);
		}
		super_column_configure();
        update_param_and_start_spi();
	}
}

public void setMaskSuperPixel(int superColumnId, int superPixelId) {
	for(int i = 0; i < 16; i++){
		setRowMask(superColumnId, superPixelId, i, 0b1111111111111111);
	}
}

void setRowMask(int superColumnId, int superPixelId, int rowId, int mask) {
    int asicId = 0;
    BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SUPER_COLUMN_NB", superColumnId);
    string path = $"SuperColumns.SuperColumn.ASIC_LSB_IF_PARALLEL_CAL{asicId}.Super_Pixel{superPixelId}.Pixel_row.Pixel_row{rowId}.Mask_0x";
	int cmd = odd_even_SC_masking_conversion(mask, superColumnId);
	//Console.WriteLine($"set {cmd} from {mask} @ {superColumnId} {superPixelId} {rowId}");
    BoardLib.SetVariable(path, odd_even_SC_masking_conversion(mask, superColumnId));
}

public void StartReadout(string filename, bool async, bool monitoring, bool UDPreadout){
    BoardLib.ActivateConfigDevice(0, true);
    BoardLib.DeviceConfigure(0, ConfigureMode.SendVerifyApply);  
    BoardLib.BoardConfigure();
    BoardLib.SetVariable("Board.DataReadoutParam.FIFO_CLR",true);
	Sync.Sleep(100);
	bool control = BoardLib.StartAcquisition(filename, async, monitoring, UDPreadout);
	Console.WriteLine("start readout");	
	if(!control){
		Console.WriteLine("Start acquisition failed");
	}
}

public void StopReadout(){
	BoardLib.StopAcquisition();
	Sync.Sleep(100);
	Console.WriteLine("stop readout");	
	// FPGAReset();
    Sync.Sleep(200);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int odd_even_SC_masking_conversion(int row_mask, int super_column_nb) {
    if(super_column_nb % 2 == 1) //For called-even super columns 
    {
        string conversion_str = Convert.ToString(row_mask, 2).PadLeft(16, '0');
        conversion_str = String.Concat( conversion_str[15],
                                        conversion_str[14],
                                        conversion_str[13],
                                        conversion_str[12],
                                        conversion_str[11],
                                        conversion_str[10],
                                        conversion_str[9],
                                        conversion_str[8],
                                        conversion_str[7],
                                        conversion_str[6],
                                        conversion_str[5],
                                        conversion_str[4],
                                        conversion_str[3],
                                        conversion_str[2],
                                        conversion_str[1],
                                        conversion_str[0]);

        return Convert.ToInt32(conversion_str, 2);
    }
    else //For called-odd super-columns
    {
    	string conversion_str = Convert.ToString(row_mask, 2).PadLeft(16, '0');
		conversion_str = String.Concat( conversion_str[8],
                                        conversion_str[9],
                                        conversion_str[10],
                                        conversion_str[11],
                                        conversion_str[12],
                                        conversion_str[13],
                                        conversion_str[14],
                                        conversion_str[15],
                                        conversion_str[0],
                                        conversion_str[1],
                                        conversion_str[2],
                                        conversion_str[3],
                                        conversion_str[4],
                                        conversion_str[5],
                                        conversion_str[6],
                                        conversion_str[7]);

        return Convert.ToInt32(conversion_str, 2);
    }
}

void update_param_and_start_spi() {
	BoardLib.UpdateUserParameters("TEST_OUT.CHIP_CONFIG_OUT");
	Sync.Sleep(5);
	BoardLib.UpdateUserParameters("TEST_OUT.SPI_START");
	Sync.Sleep(5);
}

void update_param_and_start_reset() {
	BoardLib.UpdateUserParameters("TEST_OUT.CHIP_CONFIG_OUT");
	Sync.Sleep(5);
	BoardLib.UpdateUserParameters("TEST_OUT.RESET_START");
	Sync.Sleep(5);
}

void super_column_configure() {
	Sync.Sleep(5);
	BoardLib.BoardConfigure();
	Sync.Sleep(5);
}

void BeforeMask() {
	//step1:---------------------RESET OF THE CHIP----------------------------------// C	
	int reset_sel=0;
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.RESET_SEL", reset_sel);
	bool testpulse_nreset = false;
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.TESTPULSE_NRESET", testpulse_nreset);
	BoardLib.UpdateUserParameters("TEST_OUT.CHIP_CONFIG_OUT");

	//STEP2 ---------------Super-column programming-------------------------

	int sel = 0;
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SEL", sel);
}

void AfterMask(){
	int reset_sel=0;  //to configure the masking..//sel=2 to configure the spi  
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.RESET_SEL", reset_sel);
	bool testpulse_nreset = true;
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.TESTPULSE_NRESET", testpulse_nreset);
	BoardLib.UpdateUserParameters("TEST_OUT.CHIP_CONFIG_OUT");
} 

public void SendMask() {
	BeforeMask();

	//STEP2---------------Mask generation
    int asic_nb = 0;
    for(int super_column_nb = 0; super_column_nb <13; super_column_nb++){
        BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SUPER_COLUMN_NB", super_column_nb);
        for(int SP_nb = 0; SP_nb <8; SP_nb++){
            for(int row_nb = 0; row_nb <16 ; row_nb++){
                string row_mask = mask_map[super_column_nb, SP_nb, row_nb];
                BoardLib.SetVariable(	"SuperColumns.SuperColumn.ASIC_LSB_IF_PARALLEL_CAL" + 
                asic_nb.ToString() + ".Super_Pixel" + 
                (SP_nb).ToString() + ".Pixel_row" + ".Pixel_row" + 
                row_nb.ToString() + ".Mask_0x", 
                odd_even_SC_masking_conversion(Convert.ToInt32(row_mask, 2), super_column_nb));	//odd_even_SC_masking_conversion takes care of converting the row_mask string according the parity of the super-column				
            }
        }
        super_column_configure();
        update_param_and_start_spi();
    }


	//////////FINAL STEP REMOVE THE RESET/////////////////////Send Reset/////////////////////////////////////////////////////

	AfterMask();
	
	//Sync.Sleep(500);
	//testpulse_nreset = false;
	//BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.TESTPULSE_NRESET", testpulse_nreset);
	//update_param_and_start_reset();
}

public void ConfigureSPI(int bias_testpulse_data) {


	int chip_address 		= 0;
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.CHIP_ADDRESS", chip_address);

	int super_column_nb 	= 0;
	int sel 				= 0;
	int reset_sel 			= 0;
	bool testpulse_nreset 	= false;
	int reset_start			= 0;

	// SPI Address
	int bias_preamp_cmd 	= 5;	//0b00101;
	int bias_feedback_cmd 	= 6;	//0b00110;
	int bias_disc_cmd 		= 7;	//0b00111;
	int bias_LVDS_cmd 		= 9;	//0b01001;
	int bias_load_cmd 		= 10;	//0b01010;
	int bias_idle_cmd 		= 8;	//0b00100;
	int bandgap_config_cmd 	= 11;	//0b01011;
	int bias_testpulse_cmd 	= 12;	//0b01100;
	int threshold_set_cmd 	= 13;	//0b01101;
	int threshold_offset_cmd = 29;	//0b11101;
	int bias_pixel_cmd 		= 19;	//0b10011;
	int testpulse_delay_cmd = 50;	//110010;
	int config_global_cmd 	= 30;	//0b11110;
	int readout_config_cmd 	= 31;	//0b11111;
	//int programming_word_cmd = 2;	//0b00011;
	int TDC_config_cmd 		= 3;	//0b00011;



	// SPI DATA
	int bias_preamp_data 	= 20; //75
	int bias_feedback_data 	= 40; 
	int bias_disc_data 		= 1; //1
	int bias_LVDS_data 		= 110;  //10
	int bias_load_data 		= 10;
	int bias_idle_data 		= 255;
	int bandgap_config_data = 65; //64 TP deactivated or 65 TP activated
	//int bias_testpulse_data = 8;
	int threshold_set_data 	= 0;
	int threshold_offset_data = 85; //baseline is 126 equivalent of 698 mV read from multimeter
	int bias_pixel_data 	= 255; //255
	int testpulse_delay_data = 127; //63
	int config_global_data 	= 254; //254
	int readout_config_data = 255; //255
	//int programming_word_data = 0;
	int TDC_config_data 	= 200; //200

    //---------------------RESET OF THE CHIP----------------------------------//
	reset_sel=0;
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.RESET_SEL", reset_sel);
	testpulse_nreset = false;
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.TESTPULSE_NRESET", testpulse_nreset);
	BoardLib.UpdateUserParameters("TEST_OUT.CHIP_CONFIG_OUT");

	///////////////////////////Set all DACs//////////////////////////////////////////

	sel = 2;
	
	

	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SEL", sel);

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", bias_preamp_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", bias_preamp_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", bias_feedback_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", bias_feedback_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", bias_disc_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", bias_disc_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", bias_LVDS_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", bias_LVDS_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", bias_load_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", bias_load_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", bias_idle_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", bias_idle_data);

	update_param_and_start_spi();


	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", bandgap_config_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", bandgap_config_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", bias_testpulse_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", bias_testpulse_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", threshold_set_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", threshold_set_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", threshold_offset_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", threshold_offset_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", bias_pixel_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", bias_pixel_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", testpulse_delay_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", testpulse_delay_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", config_global_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", config_global_data);

	update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", readout_config_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", readout_config_data);

	update_param_and_start_spi();

	// //Sends a single SPI command
	// BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", programming_word_cmd);
	// BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", programming_word_data);

	// update_param_and_start_spi();

	//Sends a single SPI command
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_COMMAND", TDC_config_cmd);
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.SPI_DATA", TDC_config_data);

	update_param_and_start_spi();

	///////////////////////////////Send Reset/////////////////////////////////////////////////////


	reset_sel=0;
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.RESET_SEL", reset_sel);
	testpulse_nreset = true;
	BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.TESTPULSE_NRESET", testpulse_nreset);
	BoardLib.UpdateUserParameters("TEST_OUT.CHIP_CONFIG_OUT");
	//Sync.Sleep(500);
	//testpulse_nreset = false;
	//BoardLib.SetVariable("TEST_OUT.CHIP_CONFIG_OUT.Module.TESTPULSE_NRESET", testpulse_nreset);
	//update_param_and_start_reset();

}

