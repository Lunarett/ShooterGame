#pragma once

#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Actor.h"

#if UE_EDITOR

// Get the NetMode as a string
#define GET_NET_MODE_STR() (GEngine ? (GEngine->GetNetMode(GetWorld()) == NM_ListenServer ? TEXT("Local Server") : \
                                      GEngine->GetNetMode(GetWorld()) == NM_Client ? TEXT("Remote Client") : \
                                      TEXT("Standalone")) : TEXT("Unknown"))

// Log Message to Output Log
#define LOG_TRACE(Format, ...) \
    UE_LOG(LogTemp, Log, TEXT("[%s] - [%s] - Trace: " Format), GET_NET_MODE_STR(), *FString(__FUNCTION__), ##__VA_ARGS__)

#define LOG_WARN(Format, ...) \
    UE_LOG(LogTemp, Warning, TEXT("[%s] - [%s] - Warning: " Format), GET_NET_MODE_STR(), *FString(__FUNCTION__), ##__VA_ARGS__)

#define LOG_ERROR(Format, ...) \
    UE_LOG(LogTemp, Error, TEXT("[%s] - [%s] - Error: " Format), GET_NET_MODE_STR(), *FString(__FUNCTION__), ##__VA_ARGS__)

// Log Message to Output Log and On Screen
#define LOG_TRACE_SCREEN(Format, ...) \
    { \
        LOG_TRACE(Format, ##__VA_ARGS__); \
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("[%s] - [%s] - Trace: " Format), GET_NET_MODE_STR(), *FString(__FUNCTION__), ##__VA_ARGS__)); \
    }

#define LOG_WARN_SCREEN(Format, ...) \
    { \
        LOG_WARN(Format, ##__VA_ARGS__); \
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("[%s] - [%s] - Warning: " Format), GET_NET_MODE_STR(), *FString(__FUNCTION__), ##__VA_ARGS__)); \
    }

#define LOG_ERROR_SCREEN(Format, ...) \
    { \
        LOG_ERROR(Format, ##__VA_ARGS__); \
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("[%s] - [%s] - Error: " Format), GET_NET_MODE_STR(), *FString(__FUNCTION__), ##__VA_ARGS__)); \
    }

#else

#define LOG_TRACE(Format, ...) UE_LOG(LogTemp, Log, TEXT("[Trace] - [%s]: " Format), *FString(__FUNCTION__), ##__VA_ARGS__)
#define LOG_WARN(Format, ...) UE_LOG(LogTemp, Warning, TEXT("[Warning] - [%s]: " Format), *FString(__FUNCTION__), ##__VA_ARGS__)
#define LOG_ERROR(Format, ...) UE_LOG(LogTemp, Error, TEXT("[Error] - [%s]: " Format), *FString(__FUNCTION__), ##__VA_ARGS__)

#define LOG_TRACE_SCREEN(Format, ...) LOG_TRACE(Format, ##__VA_ARGS__)
#define LOG_WARN_SCREEN(Format, ...) LOG_WARN(Format, ##__VA_ARGS__)
#define LOG_ERROR_SCREEN(Format, ...) LOG_ERROR(Format, ##__VA_ARGS__)

#endif