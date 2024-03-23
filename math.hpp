#include <cstdint>

#include <klib/math.hpp>

namespace math {
    // pi
    constexpr static double M_PI = 3.14159265358979323846;

    /**
     * @brief Constexpr function to convert radians to degrees
     * 
     * @param degrees 
     * @return constexpr float 
     */
    constexpr uint32_t rad_to_degrees(double rad){
        return rad * 180.0 / M_PI;
    }

    /**
     * @brief Constexpr function to convert degrees to radians
     * 
     * @param degrees 
     * @return constexpr float 
     */
    constexpr double degrees_to_rad(unsigned int degrees){
        return degrees * M_PI / 180.0;
    }

    /**
     * @brief Constexpr factorial implementation
     * 
     * @param n 
     * @return constexpr double 
     */
    constexpr double fac(int n){
        double res = 1;
        while(n > 0){
            res *= n;
            --n;
        }
        return res;
    }

    /**
     * @brief Constexpr sin implementation
     * 
     * @param a 
     * @return constexpr double 
     */
    constexpr double sin(double a){
        return a - klib::pow(a, 3) / fac(3)
                + klib::pow(a, 5) / fac(5) 
                - klib::pow(a, 7) / fac(7)
                + klib::pow(a, 9) / fac(9)
                - klib::pow(a, 11) / fac(11)
                + klib::pow(a, 13) / fac(13) 
                - klib::pow(a, 15) / fac(15)
                + klib::pow(a, 17) / fac(17)
                - klib::pow(a, 19) / fac(19)
                + klib::pow(a, 21) / fac(21) 
                - klib::pow(a, 23) / fac(23)
                + klib::pow(a, 25) / fac(25)
                - klib::pow(a, 27) / fac(27)
                + klib::pow(a, 29) / fac(29) 
                - klib::pow(a, 31) / fac(31)
                + klib::pow(a, 33) / fac(33)
                - klib::pow(a, 35) / fac(35)
                + klib::pow(a, 37) / fac(37) 
                - klib::pow(a, 39) / fac(39)
                + klib::pow(a, 41) / fac(41)
                - klib::pow(a, 43) / fac(43)
                + klib::pow(a, 45) / fac(45)
                - klib::pow(a, 47) / fac(47)
                + klib::pow(a, 49) / fac(49) 
                - klib::pow(a, 51) / fac(51)
                + klib::pow(a, 53) / fac(53)          
                - klib::pow(a, 55) / fac(55)
                + klib::pow(a, 57) / fac(57);
    }

    constexpr double cos(double a){
        return 1 - klib::pow(a, 2) / fac(2)
                + klib::pow(a, 4) / fac(4) 
                - klib::pow(a, 6) / fac(6)
                + klib::pow(a, 8) / fac(8)
                - klib::pow(a, 10) / fac(10)
                + klib::pow(a, 12) / fac(12) 
                - klib::pow(a, 14) / fac(14)
                + klib::pow(a, 16) / fac(16)
                - klib::pow(a, 18) / fac(18)
                + klib::pow(a, 20) / fac(20) 
                - klib::pow(a, 22) / fac(22)
                + klib::pow(a, 24) / fac(24)
                - klib::pow(a, 26) / fac(26)
                + klib::pow(a, 28) / fac(28) 
                - klib::pow(a, 30) / fac(30)
                + klib::pow(a, 32) / fac(32)
                - klib::pow(a, 34) / fac(34)
                + klib::pow(a, 36) / fac(36) 
                - klib::pow(a, 38) / fac(38)
                + klib::pow(a, 40) / fac(40)
                - klib::pow(a, 42) / fac(42)
                + klib::pow(a, 44) / fac(44) 
                - klib::pow(a, 46) / fac(46)
                + klib::pow(a, 48) / fac(48)
                - klib::pow(a, 50) / fac(50)
                + klib::pow(a, 52) / fac(52) 
                - klib::pow(a, 54) / fac(54)
                + klib::pow(a, 56) / fac(56);       
    }
}