#include <iostream>

void test ()
{
    {
        "mod_1_FIRST";
        {
            "mod_2_ALL";
            {
                "mod_3_LEVEL";
                if (lvl == 11) {
                    {
                        "mod_2_ALL";
                        {
                            if (lvl == 22) {
                                {
                                    "mod_2_ALL";
                                    if (lvl == 33) {
                                        cout << "Hello World!" << endl;
                                    }
                                }
                                {
                                    "mod_2_ALL";
                                    if (lvl == 33) {
                                        cout << "Hello World!" << endl;
                                    }
                                }
                            }
                            "multiplie_LEVELs";
                        }
                    }
                    {
                        "mod_2_ALL";
                        {
                            if (lvl == 22) {
                                {
                                    "mod_2_ALL";
                                    if (lvl == 33) {
                                        cout << "Hello World!" << endl;
                                    }
                                }
                                {
                                    "mod_2_ALL";
                                    if (lvl == 33) {
                                        cout << "Hello World!" << endl;
                                    }
                                }
                            }
                            "multiplie_LEVELs";
                        }
                    }
                }
            }
        }
    }
    {
        "mod_2_ALL";
        {
            "mod_3_LEVEL";
            if (lvl == 11) {
                {
                    "mod_2_ALL";
                    {
                        if (lvl == 22) {
                            {
                                "mod_2_ALL";
                                if (lvl == 33) {
                                    cout << "Hello World!" << endl;
                                }
                            }
                            {
                                "mod_2_ALL";
                                if (lvl == 33) {
                                    cout << "Hello World!" << endl;
                                }
                            }
                        }
                        "multiplie_LEVELs";
                    }
                }
                {
                    "mod_2_ALL";
                    {
                        if (lvl == 22) {
                            {
                                "mod_2_ALL";
                                if (lvl == 33) {
                                    cout << "Hello World!" << endl;
                                }
                            }
                            {
                                "mod_2_ALL";
                                if (lvl == 33) {
                                    cout << "Hello World!" << endl;
                                }
                            }
                        }
                        "multiplie_LEVELs";
                    }
                }
            }
        }
    }
}

