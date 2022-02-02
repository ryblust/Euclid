#pragma once

namespace euclid {

template<euclid_type Type, std::size_t Size> requires (Size <= 8)
struct EUCLID_ALIGNAS(Type, Size) Vector {
    using value_type = Type;

    constexpr std::size_t size() const noexcept {
        return Size;
    }

    constexpr Vector<float, Size> castTofloat() const noexcept requires (same_type<Type, int>) {
        return { vector.castTofloat() };
    }

    constexpr Vector<int, Size> castToint() const noexcept requires (same_type<Type, float>) {
        return { vector.castToint() };
    }

    constexpr void negative() noexcept {
        vector.negative();
    }

    constexpr float length() const noexcept {
        if (__builtin_is_constant_evaluated()) {
            return math::sqrt(static_cast<float>(this->dot(*this)));
        }
        if constexpr (same_type<Type, float>) {
    #ifdef _MSC_VER
            return _mm256_sqrt_ps(_mm256_dp_ps(*(__m256*)this, *(__m256*)this, 0xff)).m256_f32[0];
    #else
            return _mm256_sqrt_ps(_mm256_dp_ps(*(__m256*)this, *(__m256*)this, 0xff))[0];
    #endif
        }
    }

    constexpr float distance(const Vector otherVec) const noexcept {
        return ((*this) - otherVec).length();
    }

    constexpr void normalized() noexcept requires(float_point_type<Type>) {
        if (__builtin_is_constant_evaluated()) {
            vector /= this->norm();
            return;
        }
        if constexpr (Size <= 4) {
            const auto dot = _mm_dp_ps(*(__m128*)this, *(__m128*)this, 0xff);
            const auto rsq = _mm_rsqrt_ps(dot);
            const auto mul = _mm_mul_ps(*(__m128*)this, rsq);
            _mm_store_ps((float*)this, mul);
        } else {
            const auto dot = _mm256_dp_ps(*(__m256*)this, *(__m256*)this, 0xff);
            const auto rsq = _mm256_rsqrt_ps(dot);
            const auto mul = _mm256_mul_ps(*(__m256*)this, rsq);
            _mm256_store_ps((float*)this, mul);
        }
    }

    constexpr Vector<float, Size> normalize() const noexcept {
        if constexpr (same_type<Type, float>) {
            Vector normalizedVec = *this;
            normalizedVec.normalized();
            return normalizedVec;
        }
        Vector<float, Size> normalizedVec = this->castTofloat();
        normalizedVec.normalized();
        return normalizedVec;
    }

    constexpr Vector cross(const Vector otherVec) const noexcept requires(Size >= 3) {
        if (__builtin_is_constant_evaluated()) {
            Vector crossVec{}; // constexpr return value must be initialized;
            crossVec.vector.data[0] = vector.data[1] * otherVec.vector.data[2] - vector.data[2] * otherVec.vector.data[1];
            crossVec.vector.data[1] = vector.data[2] * otherVec.vector.data[0] - vector.data[0] * otherVec.vector.data[2];
            crossVec.vector.data[2] = vector.data[0] * otherVec.vector.data[1] - vector.data[1] * otherVec.vector.data[0];
            return crossVec;
        }
        Vector crossVec;
        if constexpr (same_type<Type, float>) {
            _mm256_store_ps((float*)&crossVec,
            _mm256_permute_ps(_mm256_sub_ps(_mm256_mul_ps(*(__m256*)this,
            _mm256_permute_ps(*(__m256*)&otherVec, 0b11001001)), _mm256_mul_ps(
            _mm256_permute_ps(*(__m256*)this, 0b11001001), *(__m256*)&otherVec)), 0b11001001));
        } else {
            _mm256_store_si256((__m256i*)&crossVec,
            _mm256_shuffle_epi32(_mm256_sub_epi32(_mm256_mullo_epi32(*(__m256i*)this,
            _mm256_shuffle_epi32(*(__m256i*)&otherVec, 0b11001001)), _mm256_mullo_epi32(
            _mm256_shuffle_epi32(*(__m256i*)this, 0b11001001), *(__m256i*)&otherVec)), 0b11001001));
        }
        return crossVec;
    }

    constexpr value_type dot(const Vector otherVec) const noexcept {
        return (*this) * otherVec;
    }

    constexpr value_type operator*(const Vector otherVec) const noexcept {
        if (__builtin_is_constant_evaluated()) {
            value_type dotValue{};
            for (std::size_t i = 0; i < Size; ++i) {
                dotValue += vector.data[i] * otherVec.vector.data[i];
            }
            return dotValue;
        }
        if constexpr (same_type<Type, float>) {
    #ifdef _MSC_VER
            return _mm256_dp_ps(*(__m256*)this, *(__m256*)&otherVec, 0xff).m256_f32[0];
    #else // __GUNC__
            return _mm256_dp_ps(*(__m256*)this, *(__m256*)&otherVec, 0xff)[0];
    #endif
        } else {
    #ifdef _MSC_VER
            return static_cast<int>(_mm256_dp_ps(_mm256_cvtepi32_ps(*(__m256i*)this), _mm256_cvtepi32_ps(*(__m256i*)&otherVec), 0xff).m256_f32[0]);
    #else // __GUNC__
            return static_cast<int>(_mm256_dp_ps(_mm256_cvtepi32_ps(*(__m256i*)this), _mm256_cvtepi32_ps(*(__m256i*)&otherVec), 0xff)[0]);
    #endif
        }
    }

    constexpr Vector& operator+=(const Vector otherVec) noexcept {
        vector += otherVec.vector;
        return *this;
    }

    constexpr Vector& operator-=(const Vector otherVec) noexcept {
        vector -= otherVec.vector;
        return *this;
    }

    constexpr Vector operator+(const Vector otherVec) const noexcept {
        return { vector + otherVec.vector };
    }

    constexpr Vector operator-(const Vector otherVec) const noexcept {
        return { vector - otherVec.vector };
    }

    template<arithmetic Mul> requires acceptable_loss<Type, Mul>
    constexpr Vector& operator*=(const Mul mul) noexcept  {
        vector *= mul;
        return *this;
    }

    template<arithmetic Div> requires acceptable_loss<Type, Div>
    constexpr Vector& operator/=(const Div div) noexcept {
        vector /= div;
        return *this;
    }

    template<arithmetic Mul> requires acceptable_loss<Type, Mul>
    constexpr Vector operator*(const Mul mul) const noexcept {
        return { vector * mul };
    }

    template<arithmetic Div> requires acceptable_loss<Type, Div>
    constexpr Vector operator/(const Div div) const noexcept {
        return { vector / div };
    }

    constexpr Vector operator-() const noexcept {
        return { -vector };
    }

    Array<Type, Size> vector;
};

template<arithmetic Mul, euclid_type T, std::size_t S> requires acceptable_loss<T, Mul>
EUCLID_FORCEINLINE constexpr Vector<T, S> operator*(const Mul mul, const Vector<T, S> vector) noexcept {
    return vector * mul;
}

template<euclid_type First, euclid_type... Rest> requires same_type<First, Rest...>
Vector(First, Rest...)->Vector<First, sizeof...(Rest) + 1>;

template<typename T>
struct is_vector {
    static constexpr bool value = false;
};

template<euclid_type T, std::size_t S>
struct is_vector<Vector<T, S>> {
    static constexpr bool value = true;
};

}