
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

struct Fuzzy {
    double center;
    double radius;
};

struct EmptyError {};

struct ParameterError {
    double center;
    double radius;
    const char* reason;
};

class FuzzyError : public std::exception {
    double center_;
    double radius_;
    const char* reason_;
public:
    FuzzyError(Fuzzy value, const char* reason) noexcept
        : center_(value.center), radius_(value.radius), reason_(reason) {
    }
    double center() const noexcept { return center_; }
    double radius() const noexcept { return radius_; }
    const char* what() const noexcept override { return reason_; }
};

const char* issue(Fuzzy value) noexcept {
    if (!std::isfinite(value.center) || !std::isfinite(value.radius))
        return "Fields must be finite";
    if (value.radius < 0)
        return "Radius must not be negative";
    if (std::abs(value.center) <= value.radius)
        return "Interval contains zero";
    double low = value.center - value.radius;
    double high = value.center + value.radius;
    if (!std::isfinite(low) || !std::isfinite(high))
        return "Interval endpoint overflow";
    if (!std::isfinite(1.0 / low) || !std::isfinite(1.0 / high))
        return "Reciprocal overflow";
    return nullptr;
}

Fuzzy calculate(Fuzzy value) noexcept {
    double low = 1.0 / (value.center + value.radius);
    double high = 1.0 / (value.center - value.radius);
    return { low / 2.0 + high / 2.0, high / 2.0 - low / 2.0 };
}

Fuzzy inverseAny(Fuzzy value) {
    if (const char* error = issue(value))
        throw std::domain_error(error);
    return calculate(value);
}

Fuzzy inverseNoThrow(Fuzzy value) throw() {
    
    if (const char* error = issue(value))
        throw std::domain_error(error);
    return calculate(value);
}

Fuzzy inverseStandard(Fuzzy value) throw(std::domain_error) {
    if (const char* error = issue(value))
        throw std::domain_error(error);
    return calculate(value);
}

Fuzzy inverseEmpty(Fuzzy value) throw(EmptyError) {
    if (issue(value)) throw EmptyError{};
    return calculate(value);
}

Fuzzy inverseParameters(Fuzzy value) throw(ParameterError) {
    if (const char* error = issue(value))
        throw ParameterError{ value.center, value.radius, error };
    return calculate(value);
}

Fuzzy inverseDerived(Fuzzy value) throw(FuzzyError) {
    if (const char* error = issue(value))
        throw FuzzyError(value, error);
    return calculate(value);
}

void display(const char* name, Fuzzy value) {
    std::cout << name << ": center=" << value.center
        << ", radius=" << value.radius
        << ", interval=[" << value.center - value.radius
        << ", " << value.center + value.radius << "]\n";
}

int main(int argc, char* argv[]) {
    std::cout << std::fixed << std::setprecision(6);
    if (argc == 2 && std::string(argv[1]) == "--violate-throw-empty") {
        std::set_terminate([] {
            std::cerr << "terminate: exception escaped throw()\n";
            std::_Exit(86);
            });
        
        inverseNoThrow({ 0.0, 1.0 });
        return 0;
    }

    Fuzzy good{ 5.0, 1.0 }, bad{ 1.0, 2.0 };
    try { display("No specification", inverseAny(good)); }
    catch (const std::domain_error& e) { std::cout << e.what() << '\n'; }
    display("throw() valid input", inverseNoThrow(good));
    try { display("Standard", inverseStandard(good)); }
    catch (const std::domain_error& e) { std::cout << e.what() << '\n'; }
    try { display("Empty", inverseEmpty(good)); }
    catch (const EmptyError&) { std::cout << "Empty error\n"; }
    try { display("Parameters", inverseParameters(good)); }
    catch (const ParameterError& e) { std::cout << e.reason << '\n'; }
    try { display("Derived", inverseDerived(good)); }
    catch (const FuzzyError& e) { std::cout << e.what() << '\n'; }

    try { inverseAny(bad); }
    catch (const std::domain_error& e) {
        std::cout << "Caught unrestricted: " << e.what() << '\n';
    }
    try { inverseStandard(bad); }
    catch (const std::domain_error& e) {
        std::cout << "Caught standard: " << e.what() << '\n';
    }
    try { inverseEmpty(bad); }
    catch (const EmptyError&) { std::cout << "Caught EmptyError\n"; }
    try { inverseParameters(bad); }
    catch (const ParameterError& e) {
        std::cout << "Caught ParameterError: " << e.center << ", "
            << e.radius << ", " << e.reason << '\n';
    }
    try { inverseDerived(bad); }
    catch (const FuzzyError& e) {
        std::cout << "Caught FuzzyError: " << e.center() << ", "
            << e.radius() << ", " << e.what() << '\n';
    }
    const Fuzzy cases[] = { { -5, 1 },{ 2, -1 },{ 0, 0 },
                          { std::numeric_limits<double>::infinity(), 1 } };
    for (Fuzzy value : cases) {
        try { display("Additional", inverseAny(value)); }
        catch (const std::domain_error& e) {
            std::cout << "Caught validation: " << e.what() << '\n';
        }
    }
}