#include "IR-General-Library.hpp"

int main()
{
    constexpr std::string_view DocumentsDirectory = ".\\ntust-ir-2020_hw4_v2\\docs";
    constexpr std::string_view QueriesDirectory = ".\\ntust-ir-2020_hw4_v2\\queries";
    constexpr std::string_view OutputHeader = "Query,RetrievedDocuments";
    constexpr std::string_view OutputPatheader = "plsa_result";
    constexpr std::string_view OutputPathExtension = ".txt";
    constexpr std::size_t DocumentSortCount = 1000;
    constexpr std::size_t ExpectationMaximizationCount = 32;
    constexpr std::size_t TopicCount = 32;
    constexpr double Alpha = 0.7;
    constexpr double Beta = 0.3;

    std::stringstream OutputPath;
    OutputPath << OutputPatheader << " EMCount=" << ExpectationMaximizationCount << " TopicCount=" << TopicCount << " Alpha=" << Alpha << " Beta=" << Beta << " SortCount=" << DocumentSortCount << " SpendTime=";

    auto start = std::chrono::steady_clock::now();

    // ---------------- Input file ----------------

    // List File
    auto QueriesPath = std::vector<std::filesystem::path>();
    std::copy(std::filesystem::directory_iterator(QueriesDirectory), std::filesystem::directory_iterator(), std::back_inserter(QueriesPath));

    auto DocumentsPath = std::vector<std::filesystem::path>();
    std::copy(std::filesystem::directory_iterator(DocumentsDirectory), std::filesystem::directory_iterator(), std::back_inserter(DocumentsPath));

    // Get the number of files
    auto QueriesCount = QueriesPath.size();
    auto DocumentsCount = DocumentsPath.size();

    // Get File Context
    auto QueriesContext = std::vector<std::string>(QueriesCount);
    std::transform(std::begin(QueriesPath), std::end(QueriesPath), std::begin(QueriesContext), ReadFile);

    auto DocumentsContext = std::vector<std::string>(DocumentsCount);
    std::transform(std::execution::par_unseq, std::begin(DocumentsPath), std::end(DocumentsPath), std::begin(DocumentsContext), ReadFile);

    // Get File Stem
    auto QueriesStem = std::vector<std::string>(QueriesCount);
    std::transform(std::begin(QueriesPath), std::end(QueriesPath), std::begin(QueriesStem), FileTerm<const std::filesystem::path&>);

    auto DocumentsStem = std::vector<std::string>(DocumentsCount);
    std::transform(std::begin(DocumentsPath), std::end(DocumentsPath), std::begin(DocumentsStem), FileTerm<const std::filesystem::path&>);

    // ---------------- Universal Data preprocessing ----------------

    // Counting Word Raw Count
    auto QueriesWordCountMap = std::vector<std::unordered_map<std::string, std::size_t>>(QueriesCount);
    std::transform(std::begin(QueriesContext), std::end(QueriesContext), std::begin(QueriesWordCountMap), GetWordCountMap);

    auto DocumentsWordCountMap = std::vector<std::unordered_map<std::string, std::size_t>>(DocumentsCount);
    std::transform(std::execution::par_unseq, std::begin(DocumentsContext), std::end(DocumentsContext), std::begin(DocumentsWordCountMap), GetWordCountMap);
    std::cout << "Word Count " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << std::endl;

    // It takes 0.6 seconds
    auto BackgroundWordCountMap = std::unordered_map<std::string_view, std::size_t>();
    MergeMap(std::begin(DocumentsWordCountMap), std::end(DocumentsWordCountMap), BackgroundWordCountMap, std::plus<std::size_t>());
    std::cout << "Background " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << std::endl;

    auto DocumentsWordCountSequence = std::vector<std::vector<double>>(DocumentsCount);
    std::transform(std::begin(DocumentsWordCountMap), std::end(DocumentsWordCountMap), std::begin(DocumentsWordCountSequence), [](const std::unordered_map<std::string, std::size_t>& WordCountMap)
        {
            auto WordCountSequence = std::vector<double>(std::size(WordCountMap));
            std::transform(std::begin(WordCountMap), std::end(WordCountMap), std::begin(WordCountSequence), static_cast<const std::size_t & (*)(const std::pair<std::string, std::size_t>&)>(std::get));
            return WordCountSequence;
        });

    std::size_t TopicWordIndex = 0;
    auto TopicWordIndexMap = TransformMap(BackgroundWordCountMap, [&TopicWordIndex](const std::size_t Count) { return TopicWordIndex++; });
    std::cout << "Topic Word Index " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << std::endl;

    auto DocumentsLength = std::vector<std::size_t>(DocumentsCount);
    std::transform(std::begin(DocumentsWordCountMap), std::end(DocumentsWordCountMap), std::begin(DocumentsLength), GetLength<const std::unordered_map<std::string, std::size_t>&>);

    // ---------------- Expectation Maximization Data preprocessing ----------------

    auto CountProbabilityTopicWordDocument = std::vector<std::vector<std::vector<double>>>(DocumentsCount); // cp()
    std::transform(std::begin(DocumentsWordCountMap), std::end(DocumentsWordCountMap), std::begin(CountProbabilityTopicWordDocument), [TopicCount](const std::unordered_map<std::string, std::size_t>& WordCountMap)
        {
            return std::vector<std::vector<double>>(std::size(WordCountMap), std::vector<double>(TopicCount));
        });

    auto RandomEngine = std::default_random_engine();
    auto Distribution = std::uniform_real_distribution();
    auto RandomObject = std::bind(Distribution, RandomEngine);

    // It takes 0.7 seconds
    auto TopicsProbabilityWordGivenTopic = std::vector<std::vector<double>>(TopicCount, std::vector<double>(std::size(BackgroundWordCountMap))); // Topics P(wi|Tk)
    std::for_each(std::begin(TopicsProbabilityWordGivenTopic), std::end(TopicsProbabilityWordGivenTopic), [&](std::vector<double>& ProbabilitySequence)
        {
            GenerateProbabilitySequence(ProbabilitySequence, RandomObject);
        });
    std::cout << "Probability Word Given Topic " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << std::endl;

    auto DocumentsProbabilityTopicGivenDocument = std::vector<std::vector<double>>(DocumentsCount, std::vector<double>(TopicCount)); // Documents P(Tk|dj)
    std::for_each(std::begin(DocumentsProbabilityTopicGivenDocument), std::end(DocumentsProbabilityTopicGivenDocument), [&](std::vector<double>& ProbabilitySequence)
        {
            GenerateProbabilitySequence(ProbabilitySequence, RandomObject);
        });
    std::cout << "Probability Topic Given Document " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << std::endl;

    // ---------------- Iterative Data preprocessing ----------------

    auto DocumentsWordIndexSequence = std::vector<std::vector<std::size_t>>(DocumentsCount);
    std::transform(std::begin(DocumentsWordCountMap), std::end(DocumentsWordCountMap), std::begin(DocumentsWordIndexSequence), [&](const std::unordered_map<std::string, std::size_t>& WordCountMap)
        {
            auto WordIndexSequence = std::vector<std::size_t>(std::size(WordCountMap));
            std::transform(std::begin(WordCountMap), std::end(WordCountMap), std::begin(WordIndexSequence), [&](const std::pair<std::string, std::size_t>& WordCountPair)
                {
                    return TopicWordIndexMap[WordCountPair.first];
                });

            return WordIndexSequence;
        });

    // ---------------- Expectation Maximization ----------------

    //auto ThreadCount = std::thread::hardware_concurrency();
    std::size_t ThreadCount = 6;
    auto IndexSequence = std::vector<std::atomic_size_t>(4);
    auto WaitCountSequence = std::vector<std::atomic_size_t>(3);
    auto LogLikelihoodBuffer = std::vector<double>(ThreadCount, 0.0);
    auto LogLikelihoodSequence = std::vector<double>(ExpectationMaximizationCount);
    auto ImproveSequence = std::vector<double>(ExpectationMaximizationCount);
    std::fill(std::begin(IndexSequence), std::prev(std::end(IndexSequence)), 0);
    std::fill(std::begin(WaitCountSequence), std::prev(std::end(WaitCountSequence)), 0);

    std::size_t ThreadIndex = 0;
    auto ThreadSequence = std::vector<std::thread>(ThreadCount);
    std::generate(std::begin(ThreadSequence), std::end(ThreadSequence), [&]
        {
            return std::thread(ExpectationMaximizationParallel, ThreadIndex++, ThreadCount,
                std::ref(IndexSequence), std::ref(WaitCountSequence),
                start, ExpectationMaximizationCount,
                std::cref(DocumentsLength), std::cref(DocumentsWordIndexSequence), std::cref(DocumentsWordCountSequence),
                std::ref(LogLikelihoodBuffer), std::ref(LogLikelihoodSequence), std::ref(ImproveSequence),
                std::ref(TopicsProbabilityWordGivenTopic), std::ref(DocumentsProbabilityTopicGivenDocument), std::ref(CountProbabilityTopicWordDocument));
        });

    std::for_each(std::begin(ThreadSequence), std::end(ThreadSequence), std::mem_fn(&std::thread::join));

    // ---------------- Relevance Data preprocessing ----------------

    auto DocumentsProbabilityWordGivenDocument = std::vector<std::unordered_map<std::string_view, double>>(DocumentsCount); // Documents P(wi|dj)
    std::transform(std::execution::par_unseq, std::begin(DocumentsWordCountMap), std::end(DocumentsWordCountMap), std::begin(DocumentsLength), std::begin(DocumentsProbabilityWordGivenDocument),
        TermFrequencyScheme<const std::unordered_map<std::string, std::size_t>&, std::size_t>);
    std::cout << "Probability Word Document " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << std::endl;

    auto BackgroundLength = std::reduce(std::begin(DocumentsLength), std::end(DocumentsLength));

    auto ProbabilityWordGivenBackground = TransformMap(BackgroundWordCountMap, std::bind(std::divides<double>(), std::placeholders::_1, static_cast<double>(BackgroundLength)));
    std::cout << "Probability Word Background " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << std::endl;

    // ---------------- Relevance ----------------

    // It takes 1.1 seconds
    auto QueriesDocumentsScore = std::vector<std::vector<double>>(QueriesCount);
    std::transform(std::execution::par_unseq, std::begin(QueriesWordCountMap), std::end(QueriesWordCountMap), std::begin(QueriesDocumentsScore),
        [&](const std::unordered_map<std::string, std::size_t>& QuerieWordCountMap)
        {
            return ProbabilisticLatentSemanticAnalysis(QuerieWordCountMap, DocumentsProbabilityWordGivenDocument, TopicsProbabilityWordGivenTopic, TopicWordIndexMap, DocumentsProbabilityTopicGivenDocument, ProbabilityWordGivenBackground, Alpha, Beta);
        });
    std::cout << "Core " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << std::endl;

    // ---------------- Output file ----------------

     //Use scores to sort stems
    auto QueriesDocumentsOrderedStem = std::vector<std::vector<std::reference_wrapper<const std::string>>>(QueriesCount);
    std::transform(std::begin(QueriesDocumentsScore), std::end(QueriesDocumentsScore), std::begin(QueriesDocumentsOrderedStem), [&](const std::vector<double>& DocumentsScore)
        {
            return SortStem(DocumentsScore, DocumentsStem, std::greater<double>(), DocumentSortCount);
        });
    std::cout << "Sort " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << std::endl;

    // Output file
    OutputPath << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << 's' << OutputPathExtension;
    OutputResult(OutputPath.str(), OutputHeader, QueriesStem, QueriesDocumentsOrderedStem);

    std::cout << "Output " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << std::endl;

    // ---------------- End ----------------

    return 0;
}
