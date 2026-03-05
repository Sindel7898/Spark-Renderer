/*
* Copyright 2014-2025 NVIDIA Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <NvPerfUtilities.h>
#include <nvperf_host_impl.h>
#include <NvPerfInit.h>
#include <json/json.hpp>

#include "MetricsEnumerator.h"

// this macro is needed for including rapid yaml
#define RYML_SINGLE_HDR_DEFINE_NOW
#include <ryml_all.hpp>

namespace nv { namespace perf { namespace tool {

    using namespace nlohmann;

    static void PrintUsage()
    {
        printf("Usage: MetricsEnumerator --gpu [chip_name] [--html/--csv [output_file_name]]\n");
        printf("The --gpu option is required. \n");
        printf("For output, please select either the --html or --csv option, but not both in the same execution.\n");
        printf("If output option not specified, JSON is printed to console as default output.\n");
        printf("Use \"--gpu chip_name\" to specify metrics for certain gpu.\n");
        printf("Use \"--csv output_file_name\" to generate a csv file.\n");
        printf("Use \"--html output_file_name\" to generate a html file.\n");
        printf("The output path is the current working directory.\n");

    }

    static bool ParseArguments(const int argc, const char* argv[], Options& options)
    {
        if (argc == 1)
        {
            NV_PERF_LOG_ERR(10, "The option --gpu is required but missing\n");
            PrintUsage();
            return false;
        }

        options = Options{};  
        for (int argIdx = 1; argIdx < argc; ++argIdx)
        {
            if (!strcmp(argv[argIdx], "-h") || !strcmp(argv[argIdx], "--help"))
            {
                PrintUsage();
                exit(0);
            }
            else if (!strcmp(argv[argIdx], "--gpu"))
            {
                if (argIdx + 1 < argc)  
                {
                    options.chipName = argv[++argIdx];  
                    std::transform(options.chipName.begin(), options.chipName.end(), options.chipName.begin(), ::toupper);
                    if(ChipToArch(options.chipName) == "")
                    {
                        NV_PERF_LOG_ERR(10, "Unsupported chip.\n");
                        return false;
                    }
                }
                else
                {
                    NV_PERF_LOG_ERR(10, "GPU required after %s\n", argv[argIdx]);
                    PrintUsage();
                    return false;
                }
            }
            else if (!strcmp(argv[argIdx], "--html"))
            {
                if(options.output != Options::Output::json)
                {
                    NV_PERF_LOG_ERR(10, "Please choose only one output format each execution.\n");
                    PrintUsage();
                    return false;
                }
                options.output = Options::Output::html;
                if (argIdx + 1 < argc)  
                {
                    options.outputFileName = argv[++argIdx];  
                }
            }
            else if (!strcmp(argv[argIdx], "--csv"))
            {
                if(options.output != Options::Output::json)
                {
                    NV_PERF_LOG_ERR(10, "Please choose only one output format each execution.\n");
                    PrintUsage();
                    return false;
                }
                options.output = Options::Output::csv;
                if (argIdx + 1 < argc)  
                {
                    options.outputFileName = argv[++argIdx];  
                }
            }
            else
            {
                NV_PERF_LOG_ERR(10, "Unknown or incomplete argument specified: %s\n", argv[argIdx]);
                PrintUsage();
                return false;
            }
        }
        return true;
    }

    bool InitializeState()
    {
#if defined(__linux__)
        // Set NvPerf library's search paths to current executable's dir
        const std::string exeDir =  nv::perf::utilities::GetCurrentExecutableDirectory();
        const char* pPaths[] = {
            exeDir.c_str()
        };

        NVPW_SetLibraryLoadPaths_Params params = { NVPW_SetLibraryLoadPaths_Params_STRUCT_SIZE };
        params.numPaths = sizeof(pPaths) / sizeof(pPaths[0]);
        params.ppPaths = pPaths;
        NV_PERF_LOG_INF(50, "Setting library load paths: %s\n", pPaths[0]);
        if (NVPW_SetLibraryLoadPaths(&params) != NVPA_STATUS_SUCCESS)
        {
            NV_PERF_LOG_ERR(10, "Failed to set library load paths\n");
        }
#endif
        bool nvperfStatus = InitializeNvPerf();

        if (!nvperfStatus)
        {
            NV_PERF_LOG_ERR(10, "InitializeNvPerf failed!\n");
            return false;
        }

        return true;
    }

    bool SetUserDefineMetricsAttributes(MetricConfigObject& metricConfigObject, MetricsEvaluator& metricsEvaluator)
    {
        const std::string allUserMetricsScript = metricConfigObject.GenerateScriptForAllNamespacedUserMetrics();
        if (!allUserMetricsScript.empty())
        {
            if (!metricsEvaluator.UserDefinedMetrics_Initialize())
            {
                return false; 
            }
            if (!metricsEvaluator.UserDefinedMetrics_Execute(allUserMetricsScript))
            {
                NV_PERF_LOG_ERR(50, "Failed to execute the user-defined metrics script. Is the script valid?\n");
                return false;
            }
            if (!metricsEvaluator.UserDefinedMetrics_Commit())
            {
                return false; 
            }
        }

        return true;
    }

    bool GetMetricsAttributes(const Options& options, std::vector<MetricAttribute>& allMetrics)
    {
        MetricsEvaluator metricsEvaluator;
        {
            std::vector<uint8_t> scratchBuffer;
            NVPW_MetricsEvaluator* pMetricsEvaluator = sampler::DeviceCreateMetricsEvaluator(scratchBuffer, options.chipName.c_str());
            if (!pMetricsEvaluator)
            {
                return false;
            }

            metricsEvaluator = MetricsEvaluator(pMetricsEvaluator, std::move(scratchBuffer)); // transfer ownership to metricsEvaluator
        }

        bool collectUserMetrics = false;
        if (IsChipSupportedByMetricConfigs(options.chipName))
        {
            const size_t bakedSize = MetricConfigurations::GetMetricConfigurationsSize(options.chipName.c_str());
            if (bakedSize == 0)
            {
                NV_PERF_LOG_INF(10, "User defined metrics not supported for this chip\n");
                return true;
            }
            MetricConfigObject metricConfigObject;
            if (MetricConfigurations::LoadMetricConfigObject(metricConfigObject, options.chipName, "Top_Level_Triage"))
            {
                if (SetUserDefineMetricsAttributes(metricConfigObject, metricsEvaluator))
                {
                    collectUserMetrics = true;
                }
                else
                {
                    NV_PERF_LOG_ERR(10, "Initialize user defined metrics failed\n");
                }
            }
            else
            {
                NV_PERF_LOG_INF(10, "User defined metrics not supported for this chip\n");
            }
        }

        std::vector<MetricAttribute> counterMetrics;
        std::vector<MetricAttribute> ratioMetrics;
        std::vector<MetricAttribute> throughputsMetrics;

        const std::array<MetricEnumerationInfo, 3> EnumerationInfo = {{
            {NVPW_METRIC_TYPE_COUNTER, NVPW_SUBMETRIC_NONE},
            {NVPW_METRIC_TYPE_RATIO, NVPW_SUBMETRIC_RATIO},
            {NVPW_METRIC_TYPE_THROUGHPUT, NVPW_SUBMETRIC_PCT_OF_PEAK_SUSTAINED_ELAPSED}
        }};

        auto enumerateMetricsNames = 
            [&](std::function<MetricsEnumerator(NVPW_MetricsEvaluator* pMetricsEvaluator)>&& createEnumerator, uint16_t submetric, 
                std::vector<MetricAttribute>& currentMetrics, Source sourceType) 
        {
            size_t metricIndex = (size_t)~0;
            const MetricsEnumerator enumerator = createEnumerator(metricsEvaluator);
            std::string hwUnitStr;
            NVPW_MetricType metricType;
            if (enumerator.empty() && sourceType == Source::Core) 
            {
                NV_PERF_LOG_ERR(10, "Empty metrics enumerator for core metrics\n");
            }
            for (const char* pMetricName : enumerator) 
            {
                if (pMetricName && strcmp(pMetricName, "") != 0) 
                {
                    if(GetMetricTypeAndIndex(metricsEvaluator, pMetricName, metricType, metricIndex))
                    {
                        if ((metricType != NVPW_METRIC_TYPE__COUNT) && (metricIndex != (size_t)~0))
                        {
                            hwUnitStr = pMetricName;
                            std::vector<NVPW_DimUnitFactor> actualDimUnitFactors;
                            NVPW_MetricEvalRequest metricRequest{ metricIndex, static_cast<uint8_t>(metricType), static_cast<uint8_t>(NVPW_ROLLUP_OP_AVG), submetric };
                            if(!GetMetricDimUnits(metricsEvaluator, metricRequest, actualDimUnitFactors))
                            {
                                NV_PERF_LOG_ERR(10, "Fail to get metricDimUnit\n");
                            }
                            std::string dimUnit = ToString(actualDimUnitFactors, [&](NVPW_DimUnitName dimUnit, bool plural) {
                                return ToCString(metricsEvaluator, dimUnit, plural);
                            });

                            // deal with special case
                            if(hwUnitStr.find("zcull") != std::string::npos)
                            {
                                hwUnitStr = "raster_zcull";
                            }
                            else 
                            {
                                hwUnitStr = GetMetricHwUnitStr(metricsEvaluator, metricType, metricIndex);
                            }
                            
                            std::string metricCategory = GetMetricMappedHwUnit(hwUnitStr);
                            std::string pMetricDescription = GetMetricDescription(metricsEvaluator, metricType, metricIndex);
                            std::string metricTypeStr = (metricType == NVPW_METRIC_TYPE_COUNTER ? "Counter" : (metricType == NVPW_METRIC_TYPE_RATIO ? "Ratio" : "Throughput"));
                            currentMetrics.emplace_back(pMetricName, sourceType, metricTypeStr, pMetricDescription, dimUnit, metricCategory);
                        }
                    }
                }
            }
        };

        auto enumerateMetrics = [&](Source source) {
            MetricEnumerationOption metricEnumerationOption = (source == Source::Core) ? MetricEnumerationOption::PredefinedOnly : MetricEnumerationOption::UserDefinedOnly;
            
            for (const auto& info : EnumerationInfo)
            {
                std::vector<MetricAttribute>* targetVector = nullptr;
                switch (info.metricType)
                {
                    case NVPW_METRIC_TYPE_COUNTER:
                        targetVector = &counterMetrics;
                        break;
                    case NVPW_METRIC_TYPE_RATIO:
                        targetVector = &ratioMetrics;
                        break;
                    case NVPW_METRIC_TYPE_THROUGHPUT:
                        targetVector = &throughputsMetrics;
                        break;
                }
                
                if (targetVector)
                {
                    enumerateMetricsNames(
                        [&](NVPW_MetricsEvaluator* pMetricsEvaluator) { 
                            return EnumerateMetrics(pMetricsEvaluator, info.metricType, metricEnumerationOption); 
                        },
                        static_cast<uint16_t>(info.submetric),
                        *targetVector,
                        source
                    );
                }
            }
        };

        enumerateMetrics(Source::Core);
        if (collectUserMetrics)
        {
            enumerateMetrics(Source::User);
        }

        allMetrics.insert(allMetrics.end(), std::make_move_iterator(counterMetrics.begin()), std::make_move_iterator(counterMetrics.end()));
        allMetrics.insert(allMetrics.end(), std::make_move_iterator(ratioMetrics.begin()), std::make_move_iterator(ratioMetrics.end()));
        allMetrics.insert(allMetrics.end(), std::make_move_iterator(throughputsMetrics.begin()), std::make_move_iterator(throughputsMetrics.end()));
        
        counterMetrics.clear();
        ratioMetrics.clear();
        throughputsMetrics.clear();

        return true;
    }

    bool Output(Options& options, const std::vector<MetricAttribute>& allMetrics)
    {
        const int indent = 4;
        nlohmann::ordered_json root;
        InitializeJson(options, allMetrics, root);
        const std::string jsonStr = root.dump(indent);

        // add chip arch just for default
        if(options.outputFileName == DefaultOutputFileName)
        {
            options.outputFileName += "-" + ChipToArch(options.chipName);
        }
        
        if (options.output == Options::Output::csv)
        {
            return OutputCSV(allMetrics, options.outputFileName);
        }
        else if (options.output == Options::Output::html)
        {
            return OutputHTML(jsonStr, options.outputFileName);
        }
        else if (options.output == Options::Output::json)
        {
            std::cout << jsonStr << std::endl;
        }
        return true;
    }

}}}  // nv::perf::tool

int main(const int argc, const char* argv[])
{
    using namespace nv::perf;
    using namespace nv::perf::tool;

    Options options;
    if (!ParseArguments(argc, argv, options))
    {
        NV_PERF_LOG_ERR(10, "ParseArguments failed\n");
        return -1; 
    }
    
    std::vector<MetricAttribute> allMetrics;
    if (!InitializeState())
    {
        NV_PERF_LOG_ERR(10, "InitializeState failed\n");
        return -1;
    }

    if(!GetMetricsAttributes(options, allMetrics))
    {
        NV_PERF_LOG_ERR(10, "Unsupported GPU\n");
        return -1;
    }

    if (!Output(options, allMetrics))
    {
        NV_PERF_LOG_ERR(10, "Output failed\n");
        return -1;
    }

    return 0;
}